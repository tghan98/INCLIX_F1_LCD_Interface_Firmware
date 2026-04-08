// DAC 매니저 (배터리 전압 비교용 DAC + COMP 제어)
// 구조: IDDD_ADC_Manager 모방 (비블로킹 + 상태머신)

#include "DAC_Manager.h"
#include "User_HAL_Drv.h"

//------------------------------------------------------------------------------
// 내부 상태 변수
static DAC_Manager_State_t s_state = DAC_STATE_IDLE;
static uint32_t s_dac_settle_start_tick = 0U;  // DAC 설정 시작 시간 (HAL_GetTick)
static uint8_t s_dac_manager_initialized = 0U;

// DAC 안정화 대기 시간 (ms)
#define DAC_SETTLE_TIME_MSEC (5U)

//------------------------------------------------------------------------------
/**
  * @brief   DAC 매니저 초기화 (DAC1 + COMP2 시작)
  * @param   None
  * @retval  DAC_MAN_SUCCESS 또는 DAC_MAN_SYSTEM_ERR
  */
void DAC_Manager_Init(void)
{
  DAC_HandleTypeDef *p_dac;
  COMP_HandleTypeDef *p_comp;

  // 이미 초기화되었으면 중복 실행 방지
  if (s_dac_manager_initialized != 0U)
  {
    return;
  }

  p_dac = (DAC_HandleTypeDef *)Read_DAC_HalDrive();
  p_comp = (COMP_HandleTypeDef *)Read_COMP_HalDrive();
  if ((p_dac == NULL) || (p_comp == NULL))
  {
    return;
  }

  // DAC1 채널1 시작 (PA4에서 아날로그 출력)
  (void)HAL_DAC_Start(p_dac, DAC_CHANNEL_1);

  // COMP2 시작 (PA3 vs PA4 비교)
  (void)HAL_COMP_Start(p_comp);

  // 초기화 완료
  s_dac_manager_initialized = 1U;
  s_state = DAC_STATE_IDLE;
}

//------------------------------------------------------------------------------
/**
  * @brief   DAC 기준 전압 설정 시작 (비블로킹, 상태머신 기반)
  *          IDDD_ADC_Manager_Start_Polling() 패턴 모방
  * @param   dac_value: 12-bit DAC 값 (0~4095)
  * @retval  DAC_MAN_SUCCESS: 시작 성공
  *          DAC_MAN_SYSTEM_ERR: 초기화 안 됨
  *          DAC_MAN_INVALID_PARAM: 범위 초과
  *          DAC_MAN_BUSY: 이전 요청 진행 중
  */
int32_t DAC_Manager_Start_Set_RefValue(uint16_t dac_value)
{
  // 초기화 확인
  if (s_dac_manager_initialized == 0U)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  // 범위 검사 (12-bit DAC: 0~4095)
  if (dac_value > 0x0FFFU)
  {
    return DAC_MAN_INVALID_PARAM;
  }

  // 상태 확인: IDLE이 아니면 BUSY
  if (s_state != DAC_STATE_IDLE)
  {
    return DAC_MAN_BUSY;
  }

  // DAC 출력값 설정
  if (HW_DAC_CTRL((uint32_t)dac_value) != HAL_OK)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  // 상태 변경 + 타이밍 기록
  s_dac_settle_start_tick = HAL_GetTick();
  s_state = DAC_STATE_SETTING;

  return DAC_MAN_SUCCESS;
}

//------------------------------------------------------------------------------
/**
  * @brief   DAC 값 안정화 대기 확인 (5ms 경과 확인)
  *          IDDD_ADC_Manager_Get_Polling_Result() 패턴 모방
  * @param   None
  * @retval  DAC_MAN_SUCCESS: 안정화 완료 (상태→COMPLETE)
  *          DAC_MAN_NOT_READY: 아직 대기 중
  *          DAC_MAN_SYSTEM_ERR: 상태 오류
  */
int32_t DAC_Manager_Check_Ready(void)
{
  uint32_t current_tick;
  uint32_t elapsed_ms;

  // 초기화 확인
  if (s_dac_manager_initialized == 0U)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  // 상태 확인: SETTING 상태여야 함
  if (s_state != DAC_STATE_SETTING)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  // 경과 시간 계산
  current_tick = HAL_GetTick();
  elapsed_ms = current_tick - s_dac_settle_start_tick;

  // 5ms 미만이면 아직 준비 안 됨
  if (elapsed_ms < DAC_SETTLE_TIME_MSEC)
  {
    return DAC_MAN_NOT_READY;
  }

  // 5ms 이상 경과: 안정화 완료
  s_state = DAC_STATE_COMPLETE;
  return DAC_MAN_SUCCESS;
}

//------------------------------------------------------------------------------
/**
  * @brief   COMP2 비교 결과 읽기 및 상태 복귀
  *          상태: COMPLETE → IDLE
  * @param   p_is_higher: 배터리 > DAC 여부 (1=높음, 0=낮음)
  * @retval  DAC_MAN_SUCCESS 또는 DAC_MAN_SYSTEM_ERR
  */
int32_t DAC_Manager_Get_CompareResult(uint8_t *p_is_higher)
{
  COMP_HandleTypeDef *p_comp;
  uint32_t comp_level;

  // 안전성 검사
  if (s_dac_manager_initialized == 0U || p_is_higher == NULL)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  // 상태 확인: COMPLETE 상태여야 함
  if (s_state != DAC_STATE_COMPLETE)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  p_comp = (COMP_HandleTypeDef *)Read_COMP_HalDrive();
  if (p_comp == NULL)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  // COMP2 출력 레벨 읽기
  comp_level = HAL_COMP_GetOutputLevel(p_comp);

  if (comp_level == COMP_OUTPUT_LEVEL_HIGH)
  {
    *p_is_higher = 1U;  // 배터리가 DAC보다 높음
  }
  else
  {
    *p_is_higher = 0U;  // 배터리가 DAC보다 낮음 (또는 같음)
  }

  // 상태 복귀: COMPLETE → IDLE
  s_state = DAC_STATE_IDLE;

  return DAC_MAN_SUCCESS;
}

//------------------------------------------------------------------------------
/**
  * @brief   현재 DAC 매니저 상태 조회
  * @param   None
  * @retval  DAC_Manager_State_t (IDLE / SETTING / COMPLETE)
  */
DAC_Manager_State_t DAC_Manager_Get_State(void)
{
  return s_state;
}

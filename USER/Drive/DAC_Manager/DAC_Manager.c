// DAC 매니저 (배터리 전압 비교용 DAC + COMP 제어)

#include "DAC_Manager.h"

// main.c에서 생성된 HAL 핸들 가져오기
extern DAC_HandleTypeDef hdac1;   // DAC1 핸들
extern COMP_HandleTypeDef hcomp2;  // COMP2 핸들

// 초기화 플래그 (중복 초기화 방지)
static uint8_t s_dac_manager_initialized = 0U;

/* DAC 매니저 초기화 (DAC1 + COMP2 시작) */
int32_t DAC_Manager_Init(void)
{
  // 이미 초기화되었으면 중복 실행 방지
  if (s_dac_manager_initialized != 0U)
  {
    return DAC_MAN_SUCCESS;
  }

  // DAC1 채널1 시작 (PA4에서 아날로그 출력 가능)
  if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_1) != HAL_OK)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  // COMP2 시작 (PA3 vs PA4 비교 시작)
  if (HAL_COMP_Start(&hcomp2) != HAL_OK)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  // 초기화 완료 플래그 설정
  s_dac_manager_initialized = 1U;
  return DAC_MAN_SUCCESS;
}

/* DAC 기준 전압 설정 (COMP2 비교 기준값 변경) */
int32_t DAC_Manager_Set_RefValue(uint16_t dac_value)
{
  // 초기화 확인
  if (s_dac_manager_initialized == 0U)
  {
    return DAC_MAN_SYSTEM_ERR;  // 초기화 안 됨
  }

  // 범위 검사 (12-bit DAC는 0~4095만 가능)
  if (dac_value > 0x0FFFU)  // 0x0FFF = 4095
  {
    return DAC_MAN_INVALID_PARAM;  // 범위 초과
  }

  // DAC 출력값 설정 → PA4에 아날로그 전압 출력
  // 출력 전압 = (dac_value / 4095) × 3.3V
  if (HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value) != HAL_OK)
  {
    return DAC_MAN_SYSTEM_ERR;  // 하드웨어 에러
  }

  return DAC_MAN_SUCCESS;
}

/* COMP2 비교 결과 읽기 (배터리 전압 vs DAC 기준 전압) */
int32_t DAC_Manager_Get_CompareResult(uint8_t *p_is_battery_higher_than_dac)
{
  uint32_t comp_level;

  // 안전성 검사 (초기화 + 포인터 유효성)
  if (s_dac_manager_initialized == 0U || p_is_battery_higher_than_dac == NULL)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  // COMP2 출력 레벨 읽기 (하드웨어 비교 결과)
  comp_level = HAL_COMP_GetOutputLevel(&hcomp2);

  /*
   * COMP2 비교 로직:
   * - InputPlus (PA3, 배터리) > InputMinus (PA4, DAC) → HIGH
   * - InputPlus (PA3, 배터리) < InputMinus (PA4, DAC) → LOW
   */
  if (comp_level == COMP_OUTPUT_LEVEL_HIGH)
  {
    *p_is_battery_higher_than_dac = 1U;  // 배터리가 DAC보다 높음
  }
  else if (comp_level == COMP_OUTPUT_LEVEL_LOW)
  {
    *p_is_battery_higher_than_dac = 0U;  // 배터리가 DAC보다 낮음
  }

  return DAC_MAN_SUCCESS;
}

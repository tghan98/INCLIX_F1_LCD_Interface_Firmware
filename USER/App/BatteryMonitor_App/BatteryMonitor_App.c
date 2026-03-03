// 배터리 모니터 앱 (내부 로직)

#include "BatteryMonitor_App.h"
#include "string.h"

// 이벤트 큐 (링 버퍼 구조)
static BatteryMonitor_AppEvent_t s_event_queue[BATTERYMONITOR_APP_EVENT_QUEUE_SIZE];
static uint32_t s_queue_head = 0U;    // 큐 읽기 위치
static uint32_t s_queue_tail = 0U;    // 큐 쓰기 위치
static uint32_t s_queue_count = 0U;   // 큐에 저장된 이벤트 개수

// 현재 배터리 레벨 (0~3)
static uint32_t s_current_level = BATTERY_LEVEL_HIGH;

typedef enum
{
  BAT_SCAN_STATE_INIT = 0,
  BAT_SCAN_STATE_START,          // DAC 값 설정 시작
  BAT_SCAN_STATE_WAIT_READY,     // DAC 안정화 대기 (5ms)
  BAT_SCAN_STATE_END             // 비교 결과 읽기
} BatteryScanState_t;

static BatteryScanState_t s_scan_state = BAT_SCAN_STATE_INIT;
static uint32_t s_scan_stage = 0U;
static uint8_t s_scan_result[3] = {0U, 0U, 0U};
static uint32_t s_last_cycle_tick = 0U;
static uint32_t s_stage_start_tick = 0U;

#define BAT_SCAN_CYCLE_MSEC   (100U)
#define BAT_DAC_SETTLE_MSEC   (5U)

static void BatteryMonitor_App_Que_Init(void);
static int32_t BatteryMonitor_App_Que_Push(uint32_t level);

/* 이벤트 큐 초기화 */
static void BatteryMonitor_App_Que_Init(void)
{
  s_queue_head = 0U;
  s_queue_tail = 0U;
  s_queue_count = 0U;
  memset(s_event_queue, 0, sizeof(s_event_queue));
}

/* 이벤트 큐에 배터리 레벨 추가 */
static int32_t BatteryMonitor_App_Que_Push(uint32_t level)
{
  if (s_queue_count >= BATTERYMONITOR_APP_EVENT_QUEUE_SIZE)
  {
    return -1;  // 큐가 가득 참
  }

  s_event_queue[s_queue_tail].level = level;
  s_queue_tail = (s_queue_tail + 1U) % BATTERYMONITOR_APP_EVENT_QUEUE_SIZE;  // 링 버퍼: 끝에 도달하면 0으로
  s_queue_count++;

  return 0;
}

/* 배터리 모니터 초기화 */
void BatteryMonitor_App_Init(void)
{
  BatteryMonitor_App_Que_Init();  // 이벤트 큐 초기화

  // DAC 매니저 초기화 (DAC + COMP 시작)
  DAC_Manager_Init();

  // 초기값: 배터리 완충 상태로 가정
  s_current_level = BATTERY_LEVEL_HIGH;

  s_scan_state = BAT_SCAN_STATE_INIT;
  s_scan_stage = 0U;
  s_scan_result[0] = 0U;
  s_scan_result[1] = 0U;
  s_scan_result[2] = 0U;
  s_last_cycle_tick = HAL_GetTick();
  s_stage_start_tick = s_last_cycle_tick;
}

/* 배터리 레벨 감지 및 이벤트 생성 (메인 루프에서 호출) */
void BatteryMonitor_App_Task(void)
{
  uint32_t current_tick = HAL_GetTick();
  uint32_t elapsed_ms;
  uint32_t threshold;
  uint8_t compare_result = 0U;
  uint32_t new_level;
  int32_t dac_result;

  switch (s_scan_state)
  {
    case BAT_SCAN_STATE_INIT:
      s_scan_stage = 0U;
      s_scan_result[0] = 0U;
      s_scan_result[1] = 0U;
      s_scan_result[2] = 0U;
      s_last_cycle_tick = current_tick;
      s_stage_start_tick = current_tick;
      s_scan_state = BAT_SCAN_STATE_START;
      break;

    case BAT_SCAN_STATE_START:
      // 100ms 사이클 타이밍 체크
      if (s_scan_stage == 0U)
      {
        elapsed_ms = current_tick - s_last_cycle_tick;
        if (elapsed_ms < BAT_SCAN_CYCLE_MSEC)
        {
          break;  // 아직 시간 도래 전
        }
      }

      // 단계별 DAC 임계값 결정
      if (s_scan_stage == 0U)
      {
        threshold = BAT_THRESHOLD_CRITICAL;
      }
      else if (s_scan_stage == 1U)
      {
        threshold = BAT_THRESHOLD_LOW;
      }
      else
      {
        threshold = BAT_THRESHOLD_MEDIUM;
      }

      // DAC 값 설정 시작 (비블로킹)
      dac_result = DAC_Manager_Start_Set_RefValue(threshold);
      if (dac_result == DAC_MAN_SUCCESS)
      {
        // 성공: 안정화 대기 상태로 진행
        s_stage_start_tick = current_tick;
        s_scan_state = BAT_SCAN_STATE_WAIT_READY;
      }
      else if (dac_result == DAC_MAN_BUSY)
      {
        // 이전 요청이 진행 중: 대기
      }
      else
      {
        // 에러 처리
      }
      break;

    case BAT_SCAN_STATE_WAIT_READY:
      // DAC 안정화 대기 (5ms 경과 확인)
      dac_result = DAC_Manager_Check_Ready();
      if (dac_result == DAC_MAN_SUCCESS)
      {
        // 안정화 완료: 비교 결과 읽기로 진행
        s_scan_state = BAT_SCAN_STATE_END;
      }
      else if (dac_result == DAC_MAN_NOT_READY)
      {
        // 아직 5ms 경과 전: 대기
        break;
      }
      else
      {
        // 에러: INIT으로 복귀
        s_scan_state = BAT_SCAN_STATE_INIT;
      }
      break;

    case BAT_SCAN_STATE_END:
      // COMP 비교 결과 읽기 + 상태 복귀
      dac_result = DAC_Manager_Get_CompareResult(&compare_result);
      if (dac_result != DAC_MAN_SUCCESS)
      {
        // 에러: INIT으로 복귀
        s_scan_state = BAT_SCAN_STATE_INIT;
        break;
      }

      s_scan_result[s_scan_stage] = compare_result;

      // 다음 단계로 진행 또는 결과 판정
      if (s_scan_stage < 2U)
      {
        s_scan_stage++;
        s_scan_state = BAT_SCAN_STATE_START;
        break;
      }

      // 3단계 모두 완료: 배터리 레벨 판정
      if (s_scan_result[0] == 0U)
      {
        new_level = BATTERY_LEVEL_CRITICAL;
      }
      else if (s_scan_result[1] == 0U)
      {
        new_level = BATTERY_LEVEL_LOW;
      }
      else if (s_scan_result[2] == 0U)
      {
        new_level = BATTERY_LEVEL_MEDIUM;
      }
      else
      {
        new_level = BATTERY_LEVEL_HIGH;
      }

      if (new_level != s_current_level)
      {
        s_current_level = new_level;
        BatteryMonitor_App_Que_Push(new_level);
      }

      // 다음 사이클을 위해 초기화
      s_scan_stage = 0U;
      s_last_cycle_tick = current_tick;
      s_scan_state = BAT_SCAN_STATE_START;
      break;

    default:
      s_scan_state = BAT_SCAN_STATE_INIT;
      break;
  }
}

/* 현재 배터리 레벨 반환 (0~3) */
uint32_t BatteryMonitor_App_GetLevel(void)
{
  return s_current_level;
}

/* 이벤트 큐에서 배터리 이벤트 꺼내기 (구독 패턴) */
int32_t BatteryMonitor_App_GetEvent(BatteryMonitor_AppEvent_t* event)
{
  if (s_queue_count == 0U)
  {
    return -1;  // 큐가 비어있음
  }

  *event = s_event_queue[s_queue_head];  // 가장 오래된 이벤트 읽기
  s_queue_head = (s_queue_head + 1U) % BATTERYMONITOR_APP_EVENT_QUEUE_SIZE;  // 링 버퍼 다음 위치
  s_queue_count--;

  return 0;  // 성공
}

/* 대기 중인 이벤트 개수 반환 */
uint32_t BatteryMonitor_App_GetEventCount(void)
{
  return s_queue_count;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

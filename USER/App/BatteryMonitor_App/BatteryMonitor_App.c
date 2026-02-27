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
}

/* 배터리 레벨 감지 및 이벤트 생성 (메인 루프에서 호출) */
void BatteryMonitor_App_Task(void)
{
  uint8_t result[3];  // 3개 임계값 비교 결과 저장
  uint32_t new_level;

  // 1차 비교: DAC를 2048(1.65V)로 설정 후 배터리 전압과 비교
  DAC_Manager_Set_RefValue(BAT_THRESHOLD_CRITICAL);
  HAL_Delay(5);  // DAC 안정화 대기
  DAC_Manager_Get_CompareResult(&result[0]);  // BAT > 1.65V이면 1, 아니면 0

  // 2차 비교: DAC를 2172(1.75V)로 설정 후 배터리 전압과 비교
  DAC_Manager_Set_RefValue(BAT_THRESHOLD_LOW);
  HAL_Delay(5);
  DAC_Manager_Get_CompareResult(&result[1]);  // BAT > 1.75V이면 1, 아니면 0

  // 3차 비교: DAC를 2296(1.85V)로 설정 후 배터리 전압과 비교
  DAC_Manager_Set_RefValue(BAT_THRESHOLD_MEDIUM);
  HAL_Delay(5);
  DAC_Manager_Get_CompareResult(&result[2]);  // BAT > 1.85V이면 1, 아니면 0

  /*
   * 레벨 판정 로직: 3번 비교 결과로 배터리 상태 결정
   * result[i] = 1: BAT > threshold[i]
   * result[i] = 0: BAT <= threshold[i]
   */
  if (result[0] == 0)
  {
    // BAT <= 1.65V (배터리 ≤ 3.3V)
    new_level = BATTERY_LEVEL_CRITICAL;  // 0칸: 긴급 충전 필요
  }
  else if (result[1] == 0)
  {
    // 1.65V < BAT <= 1.75V (배터리 3.3V~3.5V)
    new_level = BATTERY_LEVEL_LOW;  // 1칸: 배터리 부족
  }
  else if (result[2] == 0)
  {
    // 1.75V < BAT <= 1.85V (배터리 3.5V~3.7V)
    new_level = BATTERY_LEVEL_MEDIUM;  // 2칸: 배터리 보통
  }
  else
  {
    // BAT > 1.85V (배터리 > 3.7V)
    new_level = BATTERY_LEVEL_HIGH;  // 3칸: 배터리 충분
  }

  // 레벨이 변경되었을 때만 이벤트 큐에 추가
  if (new_level != s_current_level)
  {
    s_current_level = new_level;
    BatteryMonitor_App_Que_Push(new_level);  // UI 갱신을 위해 이벤트 발행
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

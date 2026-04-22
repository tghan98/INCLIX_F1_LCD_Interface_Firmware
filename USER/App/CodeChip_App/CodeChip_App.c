/* CodeChip app (internal logic + event queue) */

#include "CodeChip_App.h"
#include "CodeChip_Drv.h"

#include <string.h>

typedef enum
{
  CODECHIP_DETECT_IDLE = 0,
  CODECHIP_DETECT_DEBOUNCING
} CodeChip_DetectState_t;

static CodeChip_AppEvent_t    s_event_queue[CODECHIP_APP_EVENT_QUEUE_SIZE];
static uint32_t               s_queue_head;
static uint32_t               s_queue_tail;
static uint32_t               s_queue_count;
static CodeChip_DetectState_t s_detect_state;
static GPIO_PinState          s_confirmed_pin;
static GPIO_PinState          s_pending_pin;
static uint32_t               s_debounce_start_tick;

static void    CodeChip_App_Que_Init(void);
static int32_t CodeChip_App_Que_Push(CodeChip_EventType_t type);

/**
 * @brief 이벤트 큐를 초기화한다.
 */
static void CodeChip_App_Que_Init(void)
{
  s_queue_head  = 0U;
  s_queue_tail  = 0U;
  s_queue_count = 0U;
  memset(s_event_queue, 0, sizeof(s_event_queue));
}

/**
 * @brief 이벤트를 내부 큐에 적재한다. 큐가 가득 찼으면 버린다.
 * @param type 적재할 이벤트 종류.
 * @return 성공 시 0, 큐가 가득 찼으면 -1.
 */
static int32_t CodeChip_App_Que_Push(CodeChip_EventType_t type)
{
  if (s_queue_count >= CODECHIP_APP_EVENT_QUEUE_SIZE)
  {
    return -1;
  }

  s_event_queue[s_queue_tail].event        = type;
  s_event_queue[s_queue_tail].timestamp_ms = HAL_GetTick();

  s_queue_tail = (s_queue_tail + 1U) % CODECHIP_APP_EVENT_QUEUE_SIZE;
  s_queue_count++;
  return 0;
}

/**
 * @brief CodeChip App 내부 상태와 이벤트 큐를 초기화한다.
 */
void CodeChip_App_Init(void)
{
  // 1) 드라이버 레이어를 먼저 초기화한다.
  CodeChip_Drv_Init();

  // 2) 이벤트 큐와 디바운스 상태를 초기값으로 맞춘다.
  CodeChip_App_Que_Init();
  s_detect_state = CODECHIP_DETECT_IDLE;

  // 3) 초기 핀 상태를 confirmed로 기록해 첫 폴링에서 가짜 이벤트가 생기는 것을 방지한다.
  s_confirmed_pin = CodeChip_Drv_ReadDetectPin();
  s_pending_pin   = s_confirmed_pin;
}

/**
 * @brief SD card detect 핀을 폴링하고 디바운스 처리 후 이벤트를 생산한다.
 */
void CodeChip_App_Task(void)
{
  GPIO_PinState cur_pin = CodeChip_Drv_ReadDetectPin();

  switch (s_detect_state)
  {
    case CODECHIP_DETECT_IDLE:
      // 1) 핀 상태가 바뀌면 디바운스 타이머를 시작한다.
      if (cur_pin != s_confirmed_pin)
      {
        s_pending_pin         = cur_pin;
        s_debounce_start_tick = HAL_GetTick();
        s_detect_state        = CODECHIP_DETECT_DEBOUNCING;
      }
      break;

    case CODECHIP_DETECT_DEBOUNCING:
      if (cur_pin != s_pending_pin)
      {
        // 2) 디바운스 중 핀이 또 바뀌면 채터링으로 보고 타이머를 재시작한다.
        s_pending_pin         = cur_pin;
        s_debounce_start_tick = HAL_GetTick();
      }
      else if ((HAL_GetTick() - s_debounce_start_tick) >= CODECHIP_APP_DEBOUNCE_MS)
      {
        // 3) 디바운스 시간이 지나면 상태를 확정하고 이벤트를 생산한다.
        s_confirmed_pin = s_pending_pin;
        s_detect_state  = CODECHIP_DETECT_IDLE;

        if (s_confirmed_pin == CODECHIP_DRV_INSERTED_LEVEL)
        {
          (void)CodeChip_App_Que_Push(CODECHIP_EVENT_INSERTED);
        }
        else
        {
          (void)CodeChip_App_Que_Push(CODECHIP_EVENT_REMOVED);
        }
      }
      else
      {
        // 4) 디바운스 시간 미경과 — 다음 Task까지 대기한다.
      }
      break;

    default:
      s_detect_state = CODECHIP_DETECT_IDLE;
      break;
  }
}

/**
 * @brief 미처리 이벤트 큐에서 이벤트 1건을 꺼낸다.
 * @param event 이벤트를 저장할 출력 포인터.
 * @return 성공 시 0, 큐가 비어 있거나 NULL이면 -1.
 */
int32_t CodeChip_App_GetEvent(CodeChip_AppEvent_t* event)
{
  if ((event == NULL) || (s_queue_count == 0U))
  {
    return -1;
  }

  *event = s_event_queue[s_queue_head];
  s_queue_head = (s_queue_head + 1U) % CODECHIP_APP_EVENT_QUEUE_SIZE;
  s_queue_count--;
  return 0;
}

/**
 * @brief 현재 미처리 이벤트 개수를 반환한다.
 * @return 미처리 이벤트 수.
 */
uint32_t CodeChip_App_GetEventCount(void)
{
  return s_queue_count;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

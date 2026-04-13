/* Button app (internal logic + event queue) */

#include "Button_App.h"
#include "string.h"

/* Event queue (ring buffer) */
static ButtonAppEvent_t s_event_queue[BUTTON_APP_EVENT_QUEUE_SIZE];
static uint32_t s_queue_head = 0U;
static uint32_t s_queue_tail = 0U;
static uint32_t s_queue_count = 0U;

/* One button instance per ButtonId_t */
static Button_t s_buttons[BUTTON_ID_MAX];

static void Button_App_Que_Init(void);
static int32_t Button_App_Que_Push(ButtonId_t button_id, ButtonEvent_t event);

/**
 * @brief 버튼 이벤트 큐를 초기화합니다.
 * @details 큐의 head, tail, count 값을 초기 상태로 되돌리고,
 *          내부 이벤트 버퍼를 0으로 클리어합니다.
 */
static void Button_App_Que_Init(void)
{
  s_queue_head = 0U;
  s_queue_tail = 0U;
  s_queue_count = 0U;
  memset(s_event_queue, 0, sizeof(s_event_queue));
}

/**
 * @brief 버튼 이벤트를 큐에 저장합니다.
 * @param[in] button_id 이벤트가 발생한 버튼 ID입니다.
 * @param[in] event 저장할 버튼 이벤트 종류입니다.
 * @retval 0   큐 저장 성공
 * @retval -1  큐가 가득 차서 저장 실패
 */
static int32_t Button_App_Que_Push(ButtonId_t button_id, ButtonEvent_t event)
{
  if (s_queue_count >= BUTTON_APP_EVENT_QUEUE_SIZE)
  {
    return -1;
  }

  s_event_queue[s_queue_tail].button_id = button_id;
  s_event_queue[s_queue_tail].event = event;

  s_queue_tail = (s_queue_tail + 1U) % BUTTON_APP_EVENT_QUEUE_SIZE;
  s_queue_count++;

  return 0;
}

/**
 * @brief 버튼 앱을 초기화합니다.
 * @details 이벤트 큐를 초기화하고, 사용 중인 버튼 하드웨어를
 *          드라이버 레벨에서 초기 설정합니다.
 */
void Button_App_Init(void)
{
  Button_App_Que_Init();

  /* Pause button: active low */
  Button_Drv_Init(&s_buttons[BUTTON_ID_PAUSE], PC_SW_SIG_GPIO_Port, PC_SW_SIG_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 모든 버튼 상태를 주기적으로 스캔하여 이벤트를 생성합니다.
 * @details 각 버튼 인스턴스를 순회하면서 상태 변화를 확인하고,
 *          유효한 이벤트가 발생하면 내부 큐에 저장합니다.
 */
void Button_App_Task(void)
{
  ButtonEvent_t event;

  for (uint32_t i = 0U; i < BUTTON_ID_MAX; i++)
  {
    event = Button_Drv_Update(&s_buttons[i]);

    if (event != BUTTON_EVENT_NONE)
    {
      Button_App_Que_Push((ButtonId_t)i, event);
    }
  }
}

/**
 * @brief 대기 중인 버튼 이벤트 1건을 큐에서 읽어옵니다.
 * @param[out] event 읽어온 버튼 이벤트를 저장할 포인터입니다.
 * @retval 0   이벤트 읽기 성공
 * @retval -1  큐가 비어 있어 읽기 실패
 */
int32_t Button_App_GetEvent(ButtonAppEvent_t* event)
{
  if (s_queue_count == 0U)
  {
    return -1;
  }

  *event = s_event_queue[s_queue_head];
  s_queue_head = (s_queue_head + 1U) % BUTTON_APP_EVENT_QUEUE_SIZE;
  s_queue_count--;

  return 0;
}

/**
 * @brief 현재 큐에 대기 중인 버튼 이벤트 개수를 반환합니다.
 * @return uint32_t 대기 중인 이벤트 수입니다.
 */
uint32_t Button_App_GetEventCount(void)
{
  return s_queue_count;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

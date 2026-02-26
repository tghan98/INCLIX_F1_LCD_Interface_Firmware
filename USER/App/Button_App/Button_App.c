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

static void Button_App_Que_Init(void)
{
  s_queue_head = 0U;
  s_queue_tail = 0U;
  s_queue_count = 0U;
  memset(s_event_queue, 0, sizeof(s_event_queue));
}

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

void Button_App_Init(void)
{
  Button_App_Que_Init();

  /* Pause button: active low */
  Button_Drv_Init(&s_buttons[BUTTON_ID_PAUSE], PC_SW_SIG_GPIO_Port, PC_SW_SIG_Pin, GPIO_PIN_RESET);
}

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

uint32_t Button_App_GetEventCount(void)
{
  return s_queue_count;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

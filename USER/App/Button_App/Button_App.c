/**
  ******************************************************************************
  * @file           : Button_App.c
  * @brief          : 버튼 앱 구현 (L2 - 비즈니스 로직)
  *                   여러 버튼 일괄 관리, 이벤트 큐 구현
  *                   NOTE: 이 파일은 PRIVATE입니다. Button_Interface.c에서만 사용하세요.
  ******************************************************************************
  */

/* Includes */
#include "Button_App.h"
#include "string.h"

/* Private defines */

/* Private typedef */

/* Private variables */

/* 이벤트 큐 (순환 버퍼) */
static ButtonAppEvent_t s_event_queue[BUTTON_APP_EVENT_QUEUE_SIZE];
static uint32_t s_queue_head = 0U;
static uint32_t s_queue_tail = 0U;
static uint32_t s_queue_count = 0U;

/* 버튼 인스턴스들 (BUTTON_ID_MAX개 저장) */
static Button_t s_buttons[BUTTON_ID_MAX];

/* Private function prototypes */

/**
  * @brief 이벤트 큐 초기화
  * @retval None
  */
static void Button_App_Que_Init(void);

/**
  * @brief 이벤트를 큐에 추가 (내부 함수)
  * @param button_id: 버튼 ID
  * @param event: 버튼 이벤트
  * @retval 0 = 성공
  *        -1 = 큐 가득참
  */
static int32_t Button_App_Que_Push(ButtonId_t button_id, ButtonEvent_t event);

/* Function implementation */

/**
  * @brief 이벤트 큐 초기화
  * @retval None
  */
static void Button_App_Que_Init(void)
{
  s_queue_head = 0U;
  s_queue_tail = 0U;
  s_queue_count = 0U;
  
  memset(s_event_queue, 0, sizeof(s_event_queue));
}

/**
  * @brief 이벤트를 큐에 추가 (순환 버퍼에 저장)
  * @param button_id: 버튼 ID
  * @param event: 버튼 이벤트
  * @retval 0 = 성공
  *        -1 = 큐 가득참
  */
static int32_t Button_App_Que_Push(ButtonId_t button_id, ButtonEvent_t event)
{
  /* 큐가 가득 차면 거부 */
  if (s_queue_count >= BUTTON_APP_EVENT_QUEUE_SIZE)
  {
    return -1;
  }

  /* 큐의 tail 위치에 이벤트 저장 */
  s_event_queue[s_queue_tail].button_id = button_id;
  s_event_queue[s_queue_tail].event = event;

  /* tail을 다음 위치로 이동 (순환) */
  s_queue_tail = (s_queue_tail + 1U) % BUTTON_APP_EVENT_QUEUE_SIZE;
  /* 큐 아이템 개수 증가 */
  s_queue_count++;

  return 0;
}

/**
  * @brief 버튼 앱 초기화
  * @note 각 버튼을 초기화하고 큐를 준비
  * @retval None
  */
void Button_App_Init(void)
{
  /* 이벤트 큐 초기화 */
  Button_App_Que_Init();

  /* 각 버튼 드라이버 초기화 */
  /* BUTTON_ID_PAUSE: PC_SW_SIG (GPIOF, PIN_0), Active Low (GPIO_PIN_RESET) */
  Button_Drv_Init(&s_buttons[BUTTON_ID_PAUSE], PC_SW_SIG_GPIO_Port, PC_SW_SIG_Pin, GPIO_PIN_RESET);

  /* 나중에 버튼 추가 시 여기에 추가:
   * Button_Drv_Init(&s_buttons[BUTTON_ID_NEXT], ...);
   * Button_Drv_Init(&s_buttons[BUTTON_ID_PREV], ...);
   */
}

/**
  * @brief 버튼 앱 태스크 (주기적으로 호출되어야 함)
  * @note 모든 버튼을 순회하면서 GPIO 상태를 확인하고 이벤트를 생성
  * @retval None
  */
void Button_App_Task(void)
{
  ButtonEvent_t event;

  /* 모든 버튼을 순회하면서 상태머신 업데이트 */
  for (uint32_t i = 0U; i < BUTTON_ID_MAX; i++)
  {
    /* 각 버튼의 상태머신을 실행하고 이벤트 확인 */
    event = Button_Drv_Update(&s_buttons[i]);

    /* 이벤트가 발생했으면 큐에 저장 */
    if (event != BUTTON_EVENT_NONE)
    {
      Button_App_Que_Push((ButtonId_t)i, event);
    }
  }
}

/**
  * @brief 이벤트 큐에서 이벤트 하나 가져오기 (FIFO)
  * @param event: 받은 이벤트 저장 포인터
  * @retval 0 = 성공 (이벤트 받음)
  *        -1 = 큐가 비어있음
  */
int32_t Button_App_GetEvent(ButtonAppEvent_t* event)
{
  /* 큐가 비어있으면 -1 반환 */
  if (s_queue_count == 0U)
  {
    return -1;
  }

  /* head 위치의 이벤트를 복사 */
  *event = s_event_queue[s_queue_head];

  /* head를 다음 위치로 이동 (순환) */
  s_queue_head = (s_queue_head + 1U) % BUTTON_APP_EVENT_QUEUE_SIZE;
  /* 큐 아이템 개수 감소 */
  s_queue_count--;

  return 0;
}

/**
  * @brief 이벤트 큐에 남아있는 이벤트 개수 반환 (선택사항)
  * @retval 큐에 있는 이벤트 개수
  */
uint32_t Button_App_GetEventCount(void)
{
  return s_queue_count;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/**
  ******************************************************************************
  * @file           : Button_Drv.c
  * @brief          : 버튼 드라이버 구현 (L1 - 하드웨어 추상화)
  *                   GPIO 읽기, 디바운싱 처리, 상태머신 기반 이벤트 감지
  ******************************************************************************
  */

/* Includes */
#include "Button_Drv.h"

/* Private defines */

/* Private typedef */

/* Private variables */

/* Private function prototypes */

/* Function implementation */

/**
  * @brief 버튼 드라이버 초기화
  * @param btn: 초기화할 버튼 객체 포인터
  * @param port: GPIO 포트 (예: GPIOB)
  * @param pin: GPIO 핀 (예: GPIO_PIN_0)
  * @param active_level: 버튼 눌림 레벨
  * @retval None
  */
void Button_Drv_Init(Button_t* btn, GPIO_TypeDef* port, uint16_t pin, GPIO_PinState active_level)
{
  btn->port = port;
  btn->pin = pin;
  btn->active_level = active_level;
  
  btn->state = BUTTON_STATE_IDLE;
  btn->last_change_tick = 0;
}

/**
  * @brief 버튼 상태머신 업데이트 및 이벤트 반환
  * @note 상태 전이도:
  *       IDLE → (GPIO = active_level) → DEBOUNCING
  *       DEBOUNCING → (50ms 경과 후 GPIO = active_level) → PRESSED (BUTTON_EVENT_PRESSED)
  *       DEBOUNCING → (50ms 경과 후 GPIO ≠ active_level) → IDLE
  *       PRESSED → (GPIO ≠ active_level) → IDLE (BUTTON_EVENT_CLICK)
  * @param btn: 업데이트할 버튼 객체 포인터
  * @retval 발생한 이벤트 (ButtonEvent_t)
  */
ButtonEvent_t Button_Drv_Update(Button_t* btn)
{
  ButtonEvent_t event = BUTTON_EVENT_NONE;
  GPIO_PinState current_pin_state = HAL_GPIO_ReadPin(btn->port, btn->pin);
  uint32_t now = HAL_GetTick();

  switch (btn->state)
  {
    /* 초기 상태: 버튼 입력 대기 */
    case BUTTON_STATE_IDLE:
      if (current_pin_state == btn->active_level)
      {
        /* 버튼이 눌렸으므로 디바운싱 상태로 전이 */
        btn->state = BUTTON_STATE_DEBOUNCING;
        btn->last_change_tick = now;
      }
      break;

    /* 디바운싱 상태: 50ms 대기 후 상태 확인 */
    case BUTTON_STATE_DEBOUNCING:
      if ((now - btn->last_change_tick) >= BUTTON_DEBOUNCE_MS)
      {
        /* 디바운스 시간 경과 후 핀 상태 다시 확인 */
        if (current_pin_state == btn->active_level)
        {
          /* 여전히 눌려있으면 PRESSED 상태로 전이 */
          btn->state = BUTTON_STATE_PRESSED;
          event = BUTTON_EVENT_PRESSED;
        }
        else
        {
          /* 핀이 비활성이면 노이즈로 판단하고 IDLE로 돌아감 */
          btn->state = BUTTON_STATE_IDLE;
        }
      }
      break;

    /* 버튼 눌림 상태: 뗌을 대기 */
    case BUTTON_STATE_PRESSED:
      if (current_pin_state != btn->active_level)
      {
        /* 버튼이 뗄려졌으므로 IDLE로 돌아가고 클릭 이벤트 반환 */
        btn->state = BUTTON_STATE_IDLE;
        event = BUTTON_EVENT_CLICK;
      }
      break;

    /* 예상치 못한 상태: 초기화 */
    default:
      btn->state = BUTTON_STATE_IDLE;
      break;
  }

  return event;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

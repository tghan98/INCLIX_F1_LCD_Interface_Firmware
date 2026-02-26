/* Button driver (GPIO + debounce state machine) */

#include "Button_Drv.h"

void Button_Drv_Init(Button_t* btn, GPIO_TypeDef* port, uint16_t pin, GPIO_PinState active_level)
{
  btn->port = port;
  btn->pin = pin;
  btn->active_level = active_level;

  btn->state = BUTTON_STATE_IDLE;
  btn->last_change_tick = 0;
}

ButtonEvent_t Button_Drv_Update(Button_t* btn)
{
  ButtonEvent_t event = BUTTON_EVENT_NONE;
  GPIO_PinState current_pin_state = HAL_GPIO_ReadPin(btn->port, btn->pin);
  uint32_t now = HAL_GetTick();

  switch (btn->state)
  {
    case BUTTON_STATE_IDLE:
      if (current_pin_state == btn->active_level)
      {
        btn->state = BUTTON_STATE_DEBOUNCING;
        btn->last_change_tick = now;
      }
      break;

    case BUTTON_STATE_DEBOUNCING:
      if ((now - btn->last_change_tick) >= BUTTON_DEBOUNCE_MS)
      {
        if (current_pin_state == btn->active_level)
        {
          btn->state = BUTTON_STATE_PRESSED;
          event = BUTTON_EVENT_PRESSED;
        }
        else
        {
          btn->state = BUTTON_STATE_IDLE;
        }
      }
      break;

    case BUTTON_STATE_PRESSED:
      if (current_pin_state != btn->active_level)
      {
        btn->state = BUTTON_STATE_IDLE;
        event = BUTTON_EVENT_CLICK;
      }
      break;

    default:
      btn->state = BUTTON_STATE_IDLE;
      break;
  }

  return event;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

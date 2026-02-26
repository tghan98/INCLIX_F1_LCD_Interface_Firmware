/* Button driver header */

#ifndef BUTTON_DRV_H
#define BUTTON_DRV_H

#include "main.h"

#define BUTTON_DEBOUNCE_MS    50U

typedef enum
{
  BUTTON_EVENT_NONE = 0,
  BUTTON_EVENT_PRESSED,
  BUTTON_EVENT_RELEASED,
  BUTTON_EVENT_CLICK,
} ButtonEvent_t;

typedef enum
{
  BUTTON_STATE_IDLE = 0,
  BUTTON_STATE_DEBOUNCING,
  BUTTON_STATE_PRESSED,
} ButtonState_t;

typedef struct
{
  GPIO_TypeDef* port;
  uint16_t pin;
  GPIO_PinState active_level;

  ButtonState_t state;
  uint32_t last_change_tick;
} Button_t;

void Button_Drv_Init(Button_t* btn, GPIO_TypeDef* port, uint16_t pin, GPIO_PinState active_level);
ButtonEvent_t Button_Drv_Update(Button_t* btn);

#endif /* BUTTON_DRV_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

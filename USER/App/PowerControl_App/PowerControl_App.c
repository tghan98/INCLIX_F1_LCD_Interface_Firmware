#include "PowerControl_App.h"

#include "PowerControl_Drv.h"

#define LONG_PRESS_MS       3000U
#define BLINK_INTERVAL_MS   120U
#define BLINK_TOGGLE_COUNT  6U

typedef struct
{
  uint8_t active;
  uint8_t toggles_left;
  GPIO_PinState restore_state;
  uint32_t last_tick;
} led_blink_ctx_t;

static uint8_t hold_state_on;
static uint8_t press_active;
static uint8_t long_press_latched;
static uint32_t press_start_tick;
static led_blink_ctx_t led_blink;

static void led_start_blink_3x(void)
{
  led_blink.active = 1U;
  led_blink.toggles_left = BLINK_TOGGLE_COUNT;
  led_blink.restore_state = PowerControl_Drv_ReadLed();
  led_blink.last_tick = HAL_GetTick();
}

static void led_blink_task(void)
{
  uint32_t now;

  if (led_blink.active == 0U)
  {
    return;
  }

  now = HAL_GetTick();
  if ((now - led_blink.last_tick) < BLINK_INTERVAL_MS)
  {
    return;
  }

  led_blink.last_tick = now;
  PowerControl_Drv_ToggleLed();

  if (led_blink.toggles_left > 0U)
  {
    led_blink.toggles_left--;
  }

  if (led_blink.toggles_left == 0U)
  {
    PowerControl_Drv_WriteLed(led_blink.restore_state);
    led_blink.active = 0U;
  }
}

void PowerControl_App_Init(void)
{
  hold_state_on = 1U;
  press_active = 0U;
  long_press_latched = 0U;
  press_start_tick = 0U;

  led_blink.active = 0U;
  led_blink.toggles_left = 0U;
  led_blink.last_tick = 0U;
  led_blink.restore_state = GPIO_PIN_RESET;

  PowerControl_Drv_WriteHold(GPIO_PIN_SET);
  PowerControl_Drv_WriteLed(GPIO_PIN_RESET);
}

void PowerControl_App_Run(void)
{
  GPIO_PinState sw_state;

  led_blink_task();

  sw_state = PowerControl_Drv_ReadButton();

  if (sw_state == GPIO_PIN_SET)
  {
    if (press_active == 0U)
    {
      press_active = 1U;
      long_press_latched = 0U;
      press_start_tick = HAL_GetTick();
      return;
    }

    if ((long_press_latched == 0U) && ((HAL_GetTick() - press_start_tick) >= LONG_PRESS_MS))
    {
      long_press_latched = 1U;

      if (hold_state_on == 1U)
      {
        if (PowerControl_Drv_ReadUsbDetect() == GPIO_PIN_SET)
        {
          led_start_blink_3x();
        }
        else
        {
          hold_state_on = 0U;
          PowerControl_Drv_WriteHold(GPIO_PIN_RESET);
          PowerControl_Drv_WriteLed(GPIO_PIN_RESET);
        }
      }
      else
      {
        hold_state_on = 1U;
        PowerControl_Drv_WriteHold(GPIO_PIN_SET);
        PowerControl_Drv_WriteLed(GPIO_PIN_SET);
      }
    }
  }
  else
  {
    press_active = 0U;
    long_press_latched = 0U;
    press_start_tick = 0U;
  }
}

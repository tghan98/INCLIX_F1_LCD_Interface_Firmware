#include "PowerControl_Drv.h"
#include "User_HAL_Drv.h"

GPIO_PinState PowerControl_Drv_ReadButton(void)
{
  if (HW_ReadPin_PWSW_Status() == SET)
  {
    return GPIO_PIN_SET;
  }

  return GPIO_PIN_RESET;
}

GPIO_PinState PowerControl_Drv_ReadUsbDetect(void)
{
  if (HW_ReadPin_UsbDetect_Status() == SET)
  {
    return GPIO_PIN_SET;
  }

  return GPIO_PIN_RESET;
}

void PowerControl_Drv_WriteHold(GPIO_PinState state)
{
  HW_PW_OnOff((state == GPIO_PIN_SET) ? 1U : 0U);
}

void PowerControl_Drv_WriteLed(GPIO_PinState state)
{
  HW_PW_LED_ONnOFF((state == GPIO_PIN_SET) ? 1U : 0U);
}

void PowerControl_Drv_ToggleLed(void)
{
  HW_PW_LED_Toggle();
}

GPIO_PinState PowerControl_Drv_ReadLed(void)
{
  if (HW_ReadPin_PW_LED_Status() == SET)
  {
    return GPIO_PIN_SET;
  }

  return GPIO_PIN_RESET;
}

#include "PowerControl_Drv.h"

#define PC13_OUT_GPIO_Port   GPIOC
#define PC13_OUT_Pin         GPIO_PIN_13

GPIO_PinState PowerControl_Drv_ReadButton(void)
{
  return HAL_GPIO_ReadPin(PC_SW_SIG_GPIO_Port, PC_SW_SIG_Pin);
}

GPIO_PinState PowerControl_Drv_ReadUsbDetect(void)
{
  return HAL_GPIO_ReadPin(EX_PW_CK_GPIO_Port, EX_PW_CK_Pin);
}

void PowerControl_Drv_WriteHold(GPIO_PinState state)
{
  HAL_GPIO_WritePin(MPW_ONOFF_GPIO_Port, MPW_ONOFF_Pin, state);
}

void PowerControl_Drv_WriteLed(GPIO_PinState state)
{
  HAL_GPIO_WritePin(PC13_OUT_GPIO_Port, PC13_OUT_Pin, state);
}

void PowerControl_Drv_ToggleLed(void)
{
  HAL_GPIO_TogglePin(PC13_OUT_GPIO_Port, PC13_OUT_Pin);
}

GPIO_PinState PowerControl_Drv_ReadLed(void)
{
  return HAL_GPIO_ReadPin(PC13_OUT_GPIO_Port, PC13_OUT_Pin);
}

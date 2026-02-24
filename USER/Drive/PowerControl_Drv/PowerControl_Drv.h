#ifndef __POWER_CONTROL_DRV_H__
#define __POWER_CONTROL_DRV_H__

#include "main.h"

GPIO_PinState PowerControl_Drv_ReadButton(void);
GPIO_PinState PowerControl_Drv_ReadUsbDetect(void);
void PowerControl_Drv_WriteHold(GPIO_PinState state);
void PowerControl_Drv_WriteLed(GPIO_PinState state);
void PowerControl_Drv_ToggleLed(void);
GPIO_PinState PowerControl_Drv_ReadLed(void);

#endif /* __POWER_CONTROL_DRV_H__ */

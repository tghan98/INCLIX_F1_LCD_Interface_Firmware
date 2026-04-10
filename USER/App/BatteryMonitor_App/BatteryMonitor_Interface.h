/* Battery monitor interface (public API) */

#ifndef BATTERYMONITOR_INTERFACE_H
#define BATTERYMONITOR_INTERFACE_H

#include "main.h"
#include "BatteryMonitor_App.h"

void BatteryMonitor_Interface_Init(void);
void BatteryMonitor_Interface_Run(void);
int32_t BatteryMonitor_Interface_GetEvent(BatteryMonitor_AppEvent_t* event);
uint32_t BatteryMonitor_Interface_GetEventCount(void);

#endif /* BATTERYMONITOR_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

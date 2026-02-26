/* Battery monitor interface (public API) */

#include "BatteryMonitor_Interface.h"

void BatteryMonitor_Interface_Init(void)
{
  BatteryMonitor_App_Init();
}

void BatteryMonitor_Interface_Run(void)
{
  BatteryMonitor_App_Task();
}

uint32_t BatteryMonitor_Interface_GetLevel(void)
{
  return BatteryMonitor_App_GetLevel();
}

int32_t BatteryMonitor_Interface_GetEvent(BatteryMonitor_AppEvent_t* event)
{
  return BatteryMonitor_App_GetEvent(event);
}

uint32_t BatteryMonitor_Interface_GetEventCount(void)
{
  return BatteryMonitor_App_GetEventCount();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

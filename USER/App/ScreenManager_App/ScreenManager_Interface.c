/* Screen manager interface (public API) */

#include "ScreenManager_Interface.h"

int32_t ScreenManager_Interface_Init(void)
{
  return ScreenManager_App_Init();
}

int32_t ScreenManager_Interface_Run(void)
{
  return ScreenManager_App_Run();
}

int32_t ScreenManager_Interface_SubmitCommand(ScreenManager_Command_t cmd)
{
  return ScreenManager_App_SubmitCommand(cmd);
}

ScreenManager_State_t ScreenManager_Interface_GetState(void)
{
  return ScreenManager_App_GetState();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

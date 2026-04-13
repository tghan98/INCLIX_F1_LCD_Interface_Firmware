/* Power manager interface (public API) */

#include "PowerManager_Interface.h"

int32_t PowerManager_Interface_Init(void)
{
  return PowerManager_App_Init();
}

int32_t PowerManager_Interface_Run(void)
{
  return PowerManager_App_Run();
}

int32_t PowerManager_Interface_SubmitCommand(PowerManager_Command_t cmd)
{
  return PowerManager_App_SubmitCommand(cmd);
}

PowerManager_State_t PowerManager_Interface_GetState(void)
{
  return PowerManager_App_GetState();
}

PowerManager_Mode_t PowerManager_Interface_GetMode(void)
{
  return PowerManager_App_GetMode();
}

int32_t PowerManager_Interface_GetContext(PowerManager_Context_t* out_ctx)
{
  return PowerManager_App_GetContext(out_ctx);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/* Sequence manager interface (public API) */

#include "SequenceManager_Interface.h"

int32_t SequenceManager_Interface_Init(void)
{
  return SequenceManager_App_Init();
}

int32_t SequenceManager_Interface_Run(void)
{
  return SequenceManager_App_Run();
}

int32_t SequenceManager_Interface_SubmitCommand(SequenceManager_Command_t cmd)
{
  return SequenceManager_App_SubmitCommand(cmd);
}

SequenceManager_State_t SequenceManager_Interface_GetState(void)
{
  return SequenceManager_App_GetState();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
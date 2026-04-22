/* CodeChip interface — public API */

#include "CodeChip_Interface.h"
#include "CodeChip_App.h"

int32_t CodeChip_Interface_Init(void)
{
  CodeChip_App_Init();
  return 0;
}

int32_t CodeChip_Interface_Run(void)
{
  CodeChip_App_Task();
  return 0;
}

int32_t CodeChip_Interface_GetEvent(CodeChip_AppEvent_t* event)
{
  return CodeChip_App_GetEvent(event);
}

uint32_t CodeChip_Interface_GetEventCount(void)
{
  return CodeChip_App_GetEventCount();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

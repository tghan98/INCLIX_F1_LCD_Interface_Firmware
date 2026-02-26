/* Button interface (public API) */

#include "Button_Interface.h"
#include "Button_App.h"  /* private layer */

int32_t Button_Interface_Init(void)
{
  Button_App_Init();
  return 0;
}

int32_t Button_Interface_Run(void)
{
  Button_App_Task();
  return 0;
}

int32_t Button_Interface_GetEvent(ButtonAppEvent_t* event)
{
  return Button_App_GetEvent(event);
}

uint32_t Button_Interface_GetEventCount(void)
{
  return Button_App_GetEventCount();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

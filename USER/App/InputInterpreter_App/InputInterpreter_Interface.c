/* Input interpreter interface (public API) */

#include "InputInterpreter_Interface.h"
#include "InputInterpreter_App.h"

int32_t InputInterpreter_Interface_Init(void)
{
  return InputInterpreter_App_Init();
}

int32_t InputInterpreter_Interface_Run(void)
{
  return InputInterpreter_App_Run();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

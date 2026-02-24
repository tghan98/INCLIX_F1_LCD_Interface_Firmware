#include "User_Main.h"

#include "PowerControl_App.h"

int32_t User_Main_Init(void)
{
  PowerControl_App_Init();
  return 0;
}

int32_t User_Main_Run(void)
{
  PowerControl_App_Run();
  return 0;
}

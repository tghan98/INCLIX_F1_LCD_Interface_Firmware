#include "User_Main.h"

#include "App/ImageSlide_App/ImageSlide_App.h"

int32_t User_Main_Init(void)
{
  /* PowerControl is intentionally disabled while validating OLED slide show. */
  /* PowerControl_App_Init(); */
  ImageSlide_App_Init();
  return 0;
}

int32_t User_Main_Run(void)
{
  ImageSlide_App_Run();
  /* PowerControl_App_Run(); */
  return 0;
}

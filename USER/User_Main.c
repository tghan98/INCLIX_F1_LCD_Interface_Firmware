#include "User_Main.h"

#include "Button_Interface.h"
#include "BatteryMonitor_Interface.h"
#include "App/InputInterpreter_App/InputInterpreter_Interface.h"
#include "App/PowerManager_App/PowerManager_Interface.h"
#include "App/ScreenManager_App/ScreenManager_Interface.h"
#include "User_HAL_Drv.h"

int32_t User_Main_Init(void)
{
  /* 보드 ?�존 HAL ?�퍼 초기??*/
  (void)UserHAL_Config();

  /* 버튼 ?�터?�이??초기??(버튼 ?�터?�이?��? 최우?? */
  Button_Interface_Init();
  
  /* 배터�?모니?�링 초기??*/
  BatteryMonitor_Interface_Init();
  InputInterpreter_Interface_Init();

  /* ?�원 ?�책 ?�태머신 초기??*/
  PowerManager_Interface_Init();

  /* 최소 화면 상태머신 초기화 */
  ScreenManager_Interface_Init();
  
  /* PowerControl is intentionally disabled while validating OLED slide show. */
  /* PowerControl_App_Init(); */
  return 0;
}

int32_t User_Main_Run(void)
{
  /* 버튼 ?�캔 (최우?�에 ?�행?�여 ?�벤????채우�? */
  Button_Interface_Run();
  
  /* 배터�??�벨 측정 */
  BatteryMonitor_Interface_Run();
  InputInterpreter_Interface_Run();

  /* ?�원 ?�책 ?�태머신 ?�행 */
  PowerManager_Interface_Run();

  /* 최소 화면 상태머신 실행 */
  ScreenManager_Interface_Run();
  
  return 0;
}

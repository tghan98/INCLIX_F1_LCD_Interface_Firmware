#include "User_Main.h"

#include "Button_Interface.h"
#include "BatteryMonitor_Interface.h"
#include "App/BatteryDisplay_App/BatteryDisplay_App.h"

int32_t User_Main_Init(void)
{
  /* 버튼 인터페이스 초기화 (버튼 인터페이스가 최우선) */
  Button_Interface_Init();
  
  /* 배터리 모니터링 초기화 */
  BatteryMonitor_Interface_Init();
  
  /* 배터리 디스플레이 초기화 */
  BatteryDisplay_App_Init();
  
  /* PowerControl is intentionally disabled while validating OLED slide show. */
  /* PowerControl_App_Init(); */
  return 0;
}

int32_t User_Main_Run(void)
{
  /* 버튼 스캔 (최우선에 실행하여 이벤트 큐 채우기) */
  Button_Interface_Run();
  
  /* 배터리 레벨 측정 */
  BatteryMonitor_Interface_Run();
  
  /* 배터리 디스플레이 (LCD에 배터리 상태 표시) */
  BatteryDisplay_App_Run();
  
  return 0;
}

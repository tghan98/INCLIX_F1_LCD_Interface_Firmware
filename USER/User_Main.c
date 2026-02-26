#include "User_Main.h"

#include "Button_Interface.h"
#include "ImageSlide_App.h"
#include "BatteryMonitor_Interface.h"

int32_t User_Main_Init(void)
{
  /* 버튼 인터페이스 초기화 (버튼 인터페이스가 최우선) */
  Button_Interface_Init();
  
  /* 배터리 모니터링 초기화 */
  BatteryMonitor_Interface_Init();
  
  /* PowerControl is intentionally disabled while validating OLED slide show. */
  /* PowerControl_App_Init(); */
  ImageSlide_App_Init();
  return 0;
}

int32_t User_Main_Run(void)
{
  /* 버튼 스캔 (최우선에 실행하여 이벤트 큐 채우기) */
  Button_Interface_Run();
  
  /* 배터리 레벨 측정 */
  BatteryMonitor_Interface_Run();
  
  /* 슬라이드 앱 실행 (이벤트 큐 읽기) */
  ImageSlide_App_Run();
  /* PowerControl_App_Run(); */
  return 0;
}

#include "User_Main.h"

#include "Card_Insert_Test_App.h"

int32_t User_Main_Init(void)
{
  Card_Insert_Test_App_Init();
  return 0;
}

int32_t User_Main_Run(void)
{
  /* 카드 삽입 테스트 무한 루프 */
  Card_Insert_Test_App_Run();
  
  return 0;
}

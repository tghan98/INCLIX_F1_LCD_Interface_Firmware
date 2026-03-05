#include "User_Main.h"

#include "EEPROM_Test_App.h"

int32_t User_Main_Init(void)
{
  EEPROM_Test_App_Init();
  return 0;
}

int32_t User_Main_Run(void)
{
  static uint8_t s_test_done = 0U;

  if (s_test_done == 0U)
  {
    s_test_done = 1U;
    EEPROM_Test_App_RunOnce();
  }

  while (1)
  {
  }

  return 0;
}

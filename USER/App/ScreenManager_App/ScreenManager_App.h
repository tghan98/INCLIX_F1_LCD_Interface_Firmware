/* Screen manager app (internal logic) */

#ifndef SCREENMANAGER_APP_H
#define SCREENMANAGER_APP_H

#include "main.h"

typedef enum
{
  SCREENMANAGER_STATE_BOOT = 0,
  SCREENMANAGER_STATE_STANDBY,
  SCREENMANAGER_STATE_WAIT_CODECHIP,
  SCREENMANAGER_STATE_WAIT_CASSETTE,
  SCREENMANAGER_STATE_MEASURING,
  SCREENMANAGER_STATE_CALCULATING,
  SCREENMANAGER_STATE_RESULT_DISPLAY,
  SCREENMANAGER_STATE_SLEEP,
  SCREENMANAGER_STATE_BLANK,
  SCREENMANAGER_STATE_POWER_OFF_NOTICE
} ScreenManager_State_t;

typedef enum
{
  SCREENMANAGER_CMD_NONE = 0,
  SCREENMANAGER_CMD_SHOW_STANDBY,
  SCREENMANAGER_CMD_SHOW_SLEEP,
  SCREENMANAGER_CMD_SHOW_POWER_OFF_NOTICE
} ScreenManager_Command_t;

int32_t ScreenManager_App_Init(void);
int32_t ScreenManager_App_Run(void);
int32_t ScreenManager_App_SubmitCommand(ScreenManager_Command_t cmd);
ScreenManager_State_t ScreenManager_App_GetState(void);

#endif /* SCREENMANAGER_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

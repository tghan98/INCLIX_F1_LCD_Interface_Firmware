/* Input interpreter app (internal logic) */

#ifndef INPUTINTERPRETER_APP_H
#define INPUTINTERPRETER_APP_H

#include "main.h"

typedef enum
{
  INPUTINTERPRETER_TARGET_NONE = 0,
  INPUTINTERPRETER_TARGET_POWER,
  INPUTINTERPRETER_TARGET_ANALYSIS
} InputInterpreter_Target_t;

typedef enum
{
  INPUTINTERPRETER_CMD_NONE = 0,
  INPUTINTERPRETER_CMD_USER_ACTIVITY,
  INPUTINTERPRETER_CMD_ANALYSIS_START_REQUEST,
  INPUTINTERPRETER_CMD_POWER_OFF_REQUEST,
  INPUTINTERPRETER_CMD_POWER_BATTERY_LOW,
  INPUTINTERPRETER_CMD_POWER_BATTERY_CRITICAL
} InputInterpreter_Command_t;

typedef struct
{
  InputInterpreter_Target_t target;
  InputInterpreter_Command_t cmd;
  uint32_t param0;
  uint32_t timestamp_ms;
} InputInterpreter_TranslatedCmd_t;

int32_t InputInterpreter_App_Init(void);
int32_t InputInterpreter_App_Run(void);
int32_t InputInterpreter_App_GetLastTranslated(InputInterpreter_TranslatedCmd_t* out_cmd);

#endif /* INPUTINTERPRETER_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

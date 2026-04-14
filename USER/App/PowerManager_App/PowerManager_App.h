/* Power manager app (internal logic) */

#ifndef POWERMANAGER_APP_H
#define POWERMANAGER_APP_H

#include "main.h"

typedef enum
{
  POWERMANAGER_STATE_BOOT = 0,
  POWERMANAGER_STATE_STANDBY,
  POWERMANAGER_STATE_SLEEP,
  POWERMANAGER_STATE_POWER_OFF_NOTICE,
  POWERMANAGER_STATE_POWER_OFF
} PowerManager_State_t;

typedef enum
{
  POWERMANAGER_MODE_UNKNOWN = 0,
  POWERMANAGER_MODE_CHARGING,
  POWERMANAGER_MODE_DISCHARGING
} PowerManager_Mode_t;

typedef enum
{
  POWERMANAGER_CMD_NONE = 0,
  POWERMANAGER_CMD_ACTIVITY,
  POWERMANAGER_CMD_FORCE_SLEEP,
  POWERMANAGER_CMD_WAKEUP,
  POWERMANAGER_CMD_BATTERY_LOW,
  POWERMANAGER_CMD_BATTERY_CRITICAL,
  POWERMANAGER_CMD_CHARGER_ATTACHED,
  POWERMANAGER_CMD_CHARGER_DETACHED
} PowerManager_Command_t;

typedef struct
{
  PowerManager_State_t state;
  PowerManager_Mode_t mode;
  uint32_t state_enter_tick;
  uint32_t last_activity_tick;
  uint32_t idle_timeout_ms;
  uint32_t sleep_timeout_ms;
  uint32_t power_off_notice_timeout_ms;
  uint8_t battery_low_latched;
  uint8_t battery_critical_latched;
} PowerManager_Context_t;

int32_t PowerManager_App_Init(void);
int32_t PowerManager_App_Run(void);
int32_t PowerManager_App_SubmitCommand(PowerManager_Command_t cmd);
PowerManager_State_t PowerManager_App_GetState(void);
PowerManager_Mode_t PowerManager_App_GetMode(void);
int32_t PowerManager_App_GetContext(PowerManager_Context_t* out_ctx);

#endif /* POWERMANAGER_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

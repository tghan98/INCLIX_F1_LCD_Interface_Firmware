/* Power manager interface (public API) */

#ifndef POWERMANAGER_INTERFACE_H
#define POWERMANAGER_INTERFACE_H

#include "main.h"
#include "PowerManager_App.h"

int32_t PowerManager_Interface_Init(void);
int32_t PowerManager_Interface_Run(void);
int32_t PowerManager_Interface_SubmitCommand(PowerManager_Command_t cmd);
PowerManager_State_t PowerManager_Interface_GetState(void);
PowerManager_Mode_t PowerManager_Interface_GetMode(void);
int32_t PowerManager_Interface_GetContext(PowerManager_Context_t* out_ctx);

#endif /* POWERMANAGER_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

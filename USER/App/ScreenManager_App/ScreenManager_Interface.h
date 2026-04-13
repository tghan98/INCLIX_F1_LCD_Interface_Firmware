/* Screen manager interface (public API) */

#ifndef SCREENMANAGER_INTERFACE_H
#define SCREENMANAGER_INTERFACE_H

#include "main.h"
#include "ScreenManager_App.h"

int32_t ScreenManager_Interface_Init(void);
int32_t ScreenManager_Interface_Run(void);
int32_t ScreenManager_Interface_SubmitCommand(ScreenManager_Command_t cmd);
ScreenManager_State_t ScreenManager_Interface_GetState(void);

#endif /* SCREENMANAGER_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/* Button app header (internal use) */

#ifndef BUTTON_APP_H
#define BUTTON_APP_H

#include "main.h"
#include "Button_Drv.h"
#include "Button_Interface.h"

#define BUTTON_APP_EVENT_QUEUE_SIZE    10U

void Button_App_Init(void);
void Button_App_Task(void);
int32_t Button_App_GetEvent(ButtonAppEvent_t* event);
uint32_t Button_App_GetEventCount(void);

#endif /* BUTTON_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

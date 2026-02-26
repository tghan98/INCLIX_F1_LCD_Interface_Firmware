/* Button interface header (public API) */

#ifndef BUTTON_INTERFACE_H
#define BUTTON_INTERFACE_H

#include "main.h"
#include "Button_Drv.h"  /* for ButtonEvent_t */

/* System button IDs */
typedef enum
{
  BUTTON_ID_PAUSE = 0,
  BUTTON_ID_MAX
} ButtonId_t;

/* Event message passed from button app to other modules */
typedef struct
{
  ButtonId_t button_id;
  ButtonEvent_t event;
} ButtonAppEvent_t;

int32_t Button_Interface_Init(void);
int32_t Button_Interface_Run(void);
int32_t Button_Interface_GetEvent(ButtonAppEvent_t* event);
uint32_t Button_Interface_GetEventCount(void);

#endif /* BUTTON_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

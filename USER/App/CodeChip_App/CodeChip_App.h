/* CodeChip app header (internal use) */

#ifndef CODECHIP_APP_H
#define CODECHIP_APP_H

#include "main.h"
#include "CodeChip_Interface.h"

#define CODECHIP_APP_EVENT_QUEUE_SIZE  4U
#define CODECHIP_APP_DEBOUNCE_MS       30U

void     CodeChip_App_Init(void);
void     CodeChip_App_Task(void);
int32_t  CodeChip_App_GetEvent(CodeChip_AppEvent_t* event);
uint32_t CodeChip_App_GetEventCount(void);

#endif /* CODECHIP_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

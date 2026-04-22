/* CodeChip interface — public API for raw SD card insertion/removal event production */

#ifndef CODECHIP_INTERFACE_H
#define CODECHIP_INTERFACE_H

#include "main.h"

typedef enum
{
  CODECHIP_EVENT_NONE = 0,
  CODECHIP_EVENT_INSERTED,
  CODECHIP_EVENT_REMOVED
} CodeChip_EventType_t;

typedef struct
{
  CodeChip_EventType_t event;
  uint32_t             timestamp_ms;
} CodeChip_AppEvent_t;

int32_t  CodeChip_Interface_Init(void);
int32_t  CodeChip_Interface_Run(void);
int32_t  CodeChip_Interface_GetEvent(CodeChip_AppEvent_t* event);
uint32_t CodeChip_Interface_GetEventCount(void);

#endif /* CODECHIP_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

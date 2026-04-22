/* Sequence manager interface (public API) */

#ifndef SEQUENCEMANAGER_INTERFACE_H
#define SEQUENCEMANAGER_INTERFACE_H

#include "main.h"
#include "SequenceManager_App.h"

int32_t SequenceManager_Interface_Init(void);
int32_t SequenceManager_Interface_Run(void);
int32_t SequenceManager_Interface_SubmitCommand(SequenceManager_Command_t cmd);
SequenceManager_State_t SequenceManager_Interface_GetState(void);

#endif /* SEQUENCEMANAGER_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
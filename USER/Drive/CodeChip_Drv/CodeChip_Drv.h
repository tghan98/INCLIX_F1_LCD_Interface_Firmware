/* CodeChip driver — board-dependent Micro SD card detect GPIO definitions */

#ifndef CODECHIP_DRV_H
#define CODECHIP_DRV_H

#include "main.h"

/* SD card detect 핀 — 보드 회로 Chip_CD: PB2, active-low (삽입 시 GND) */
#define CODECHIP_DRV_DETECT_PORT     GPIOB
#define CODECHIP_DRV_DETECT_PIN      GPIO_PIN_2
#define CODECHIP_DRV_INSERTED_LEVEL  GPIO_PIN_RESET

void          CodeChip_Drv_Init(void);
GPIO_PinState CodeChip_Drv_ReadDetectPin(void);

#endif /* CODECHIP_DRV_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

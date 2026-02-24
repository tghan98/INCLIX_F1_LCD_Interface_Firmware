#ifndef __OLED_SSD1322_DRV_H__
#define __OLED_SSD1322_DRV_H__

#include "main.h"

#define OLED_SSD1322_WIDTH            256U
#define OLED_SSD1322_HEIGHT           64U
#define OLED_SSD1322_FRAME_BYTES      ((OLED_SSD1322_WIDTH * OLED_SSD1322_HEIGHT) / 2U)

void OLED_SSD1322_Drv_Init(void);
void OLED_SSD1322_Drv_WriteFrame(const uint8_t *frame);

#endif /* __OLED_SSD1322_DRV_H__ */

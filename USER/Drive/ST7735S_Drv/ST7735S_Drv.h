#ifndef __ST7735S_DRV_H__
#define __ST7735S_DRV_H__

#include "main.h"

/* Keep legacy names for app compatibility (source assets are 4bpp, 256x64). */
#define ST7735S_DRV_WIDTH            256U
#define ST7735S_DRV_HEIGHT           64U
#define ST7735S_DRV_FRAME_BYTES      ((ST7735S_DRV_WIDTH * ST7735S_DRV_HEIGHT) / 2U)

void ST7735S_Drv_Init(void);
void ST7735S_Drv_WriteFrame(const uint8_t *frame);

#endif /* __ST7735S_DRV_H__ */

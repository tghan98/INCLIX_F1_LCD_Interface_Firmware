#ifndef __ST7735S_DRV_H__
#define __ST7735S_DRV_H__

#include "main.h"

/* Legacy frame format used by existing apps/assets (256x64, 4bpp packed). */
#define ST7735S_DRV_WIDTH         256U
#define ST7735S_DRV_HEIGHT        64U
#define ST7735S_DRV_FRAME_BYTES   ((ST7735S_DRV_WIDTH * ST7735S_DRV_HEIGHT) / 2U)

#define ST7735S_PANEL_WIDTH       128U
#define ST7735S_PANEL_HEIGHT      96U

/* Visible safe area measured on hardware. */
#define ST7735S_VIEW_X_MIN        21U
#define ST7735S_VIEW_X_MAX        105U
#define ST7735S_VIEW_Y_MIN        2U
#define ST7735S_VIEW_Y_MAX        95U
#define ST7735S_VIEW_WIDTH        (ST7735S_VIEW_X_MAX - ST7735S_VIEW_X_MIN + 1U)
#define ST7735S_VIEW_HEIGHT       (ST7735S_VIEW_Y_MAX - ST7735S_VIEW_Y_MIN + 1U)

void ST7735S_Drv_Init(void);
void ST7735S_Drv_WriteFrame(const uint8_t *frame);
void ST7735S_Drv_Clear(uint16_t rgb565);
void ST7735S_Drv_DrawChar5x7(uint16_t x, uint16_t y, char ch, uint16_t fg_rgb565, uint16_t bg_rgb565);
void ST7735S_Drv_DrawString5x7(uint16_t x, uint16_t y, const char *text, uint16_t fg_rgb565, uint16_t bg_rgb565);
void ST7735S_Drv_TestCornerPixels(void);

#endif /* __ST7735S_DRV_H__ */

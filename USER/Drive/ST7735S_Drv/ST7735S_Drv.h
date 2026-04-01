#ifndef __ST7735S_DRV_H__
#define __ST7735S_DRV_H__

#include "main.h"

/* Keep legacy names for app compatibility (source assets are 4bpp, 256x64). */
#define ST7735S_DRV_WIDTH            256U
#define ST7735S_DRV_HEIGHT           64U
#define ST7735S_DRV_FRAME_BYTES      ((ST7735S_DRV_WIDTH * ST7735S_DRV_HEIGHT) / 2U)

/* LCD panel dimensions */
#define ST7735S_PANEL_WIDTH          128U
#define ST7735S_PANEL_HEIGHT         96U

/* Visible safe area (bezel-safe bounds) */
#define ST7735S_VIEW_X_MIN           21U
#define ST7735S_VIEW_X_MAX           107U
#define ST7735S_VIEW_Y_MIN           2U
#define ST7735S_VIEW_Y_MAX           95U

void ST7735S_Drv_Init(void);
void ST7735S_Drv_WriteFrame(const uint8_t *frame);
void ST7735S_Drv_Clear(uint16_t rgb565);
void ST7735S_Drv_DrawChar3x5(uint16_t x, uint16_t y, char ch, uint16_t fg_rgb565, uint16_t bg_rgb565);
void ST7735S_Drv_DrawString3x5(uint16_t x, uint16_t y, const char *text, uint16_t fg_rgb565, uint16_t bg_rgb565);

#endif /* __ST7735S_DRV_H__ */

#ifndef __ST7735S_DRV_H__
#define __ST7735S_DRV_H__

#include "main.h"

/* 드라이버 입력 프레임 크기입니다. (4bpp, 128x97) */
#define ST7735S_DRV_WIDTH            128U
#define ST7735S_DRV_HEIGHT           97U
#define ST7735S_DRV_FRAME_BYTES      ((ST7735S_DRV_WIDTH * ST7735S_DRV_HEIGHT) / 2U)

/* LCD 실제 패널 크기입니다. */
#define ST7735S_PANEL_WIDTH          128U
#define ST7735S_PANEL_HEIGHT         97U

/* 실제로 안정적으로 보이는 표시 영역입니다. (패널 절대 좌표 기준) */
#define ST7735S_VIEW_X_MIN           23U
#define ST7735S_VIEW_X_MAX           107U
#define ST7735S_VIEW_Y_MIN           2U
#define ST7735S_VIEW_Y_MAX           95U

/* 논리 좌표 크기입니다. ((0,0)은 표시 영역의 왼쪽 위) */
#define ST7735S_LOGICAL_WIDTH        ((ST7735S_VIEW_X_MAX) - (ST7735S_VIEW_X_MIN) + 1U)   /* 85 */
#define ST7735S_LOGICAL_HEIGHT       ((ST7735S_VIEW_Y_MAX) - (ST7735S_VIEW_Y_MIN) + 1U)   /* 94 */

/* LCD 초기화 */
void ST7735S_Drv_Init(void);
/* 4bpp 프레임 출력 (입력 버퍼 크기: ST7735S_DRV_FRAME_BYTES = 6272 bytes) */
void ST7735S_Drv_WriteFrame(const uint8_t *frame);
/* 화면 전체를 지정 색으로 채움 */
void ST7735S_Drv_Clear(uint16_t rgb565);
/* 3x5 글자 1개 출력 */
void ST7735S_Drv_DrawChar3x5(uint16_t x, uint16_t y, char ch, uint16_t fg_rgb565, uint16_t bg_rgb565);
/* 3x5 문자열 출력 */
void ST7735S_Drv_DrawString3x5(uint16_t x, uint16_t y, const char *text, uint16_t fg_rgb565, uint16_t bg_rgb565);

#endif /* __ST7735S_DRV_H__ */

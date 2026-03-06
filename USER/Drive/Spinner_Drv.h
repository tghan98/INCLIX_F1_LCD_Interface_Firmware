#ifndef SPINNER_DRV_H
#define SPINNER_DRV_H

#include <stdint.h>

/* Spinner sprite configuration */
#define SPINNER_SPRITE_W         (14)
#define SPINNER_SPRITE_H         (17)
#define SPINNER_DST_SIZE         (18)
#define SPINNER_CENTER_X         (128)
#define SPINNER_CENTER_Y         (32)
#define SPINNER_FIX_SCALE        (1024)
#define SPINNER_FRAMES           (60)

/* Driver functions */
void Spinner_Drv_SetPixel4bpp(uint8_t *frame, uint32_t x, uint32_t y, uint8_t gray);
void Spinner_Drv_DrawRotatingSprite(uint8_t *frame, uint32_t angle_idx);
void Spinner_Drv_DrawNumber(uint8_t *frame, uint32_t x, uint32_t y, uint32_t num);
void Spinner_Drv_DrawText(uint8_t *frame, uint32_t x, uint32_t y, const char *text);

#endif /* SPINNER_DRV_H */

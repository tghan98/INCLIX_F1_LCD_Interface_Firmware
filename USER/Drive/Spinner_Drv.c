#include "Spinner_Drv.h"
#include "OLED_SSD1322_Drv.h"

/* 6-degree step lookup table, fixed-point scale 1024 */
static const int16_t s_cos_lut[SPINNER_FRAMES] = {
  1024, 1018, 1002, 974, 935, 887, 828, 761, 685, 602,
  512, 416, 316, 213, 107, 0, -107, -213, -316, -416,
  -512, -602, -685, -761, -828, -887, -935, -974, -1002, -1018,
  -1024, -1018, -1002, -974, -935, -887, -828, -761, -685, -602,
  -512, -416, -316, -213, -107, 0, 107, 213, 316, 416,
  512, 602, 685, 761, 828, 887, 935, 974, 1002, 1018
};

static const int16_t s_sin_lut[SPINNER_FRAMES] = {
  0, 107, 213, 316, 416, 512, 602, 685, 761, 828,
  887, 935, 974, 1002, 1018, 1024, 1018, 1002, 974, 935,
  887, 828, 761, 685, 602, 512, 416, 316, 213, 107,
  0, -107, -213, -316, -416, -512, -602, -685, -761, -828,
  -887, -935, -974, -1002, -1018, -1024, -1018, -1002, -974, -935,
  -887, -828, -761, -685, -602, -512, -416, -316, -213, -107
};

/* 14x17 hourglass sprite (from Desktop image) */
static const uint8_t s_hourglass_sprite[SPINNER_SPRITE_H][SPINNER_SPRITE_W] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {0,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {0,1,1,1,1,1,1,1,1,1,1,1,1,0},
  {0,0,1,1,0,0,0,0,0,0,1,1,0,0},
  {0,0,0,1,1,0,0,0,0,1,1,0,0,0},
  {0,0,0,0,1,1,0,0,1,1,0,0,0,0},
  {0,0,0,0,1,1,0,0,1,1,0,0,0,0},
  {0,0,0,0,1,1,0,0,1,1,0,0,0,0},
  {0,0,0,0,1,1,0,0,1,1,0,0,0,0},
  {0,0,0,0,1,1,0,0,1,1,0,0,0,0},
  {0,0,0,1,1,0,0,0,0,1,1,0,0,0},
  {0,0,1,1,0,0,0,0,0,0,1,1,0,0},
  {0,1,1,0,0,0,0,0,0,0,0,1,1,0},
  {0,1,1,0,0,0,0,0,0,0,0,1,1,0},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

void Spinner_Drv_SetPixel4bpp(uint8_t *frame, uint32_t x, uint32_t y, uint8_t gray)
{
  uint32_t byte_index;
  uint8_t nibble;

  if ((x >= OLED_SSD1322_WIDTH) || (y >= OLED_SSD1322_HEIGHT))
  {
    return;
  }

  byte_index = y * (OLED_SSD1322_WIDTH / 2U) + (x / 2U);
  nibble = (uint8_t)(gray & 0x0FU);

  if ((x & 1U) == 0U)
  {
    frame[byte_index] = (frame[byte_index] & 0x0FU) | (uint8_t)(nibble << 4);
  }
  else
  {
    frame[byte_index] = (frame[byte_index] & 0xF0) | nibble;
  }
}

void Spinner_Drv_DrawRotatingSprite(uint8_t *frame, uint32_t angle_idx)
{
  int32_t dx;
  int32_t dy;
  int32_t src_x;
  int32_t src_y;
  int16_t c;
  int16_t s;
  int32_t half_dst;
  int32_t src_cx;
  int32_t src_cy;

  c = s_cos_lut[angle_idx % SPINNER_FRAMES];
  s = s_sin_lut[angle_idx % SPINNER_FRAMES];
  half_dst = SPINNER_DST_SIZE / 2;
  src_cx = SPINNER_SPRITE_W / 2;
  src_cy = SPINNER_SPRITE_H / 2;

  for (dy = -half_dst; dy < half_dst; dy++)
  {
    for (dx = -half_dst; dx < half_dst; dx++)
    {
      /* Inverse rotate destination pixel into source space */
      src_x = ((c * dx) + (s * dy)) / SPINNER_FIX_SCALE + src_cx;
      src_y = ((-s * dx) + (c * dy)) / SPINNER_FIX_SCALE + src_cy;

      if ((src_x >= 0) && (src_x < SPINNER_SPRITE_W) && (src_y >= 0) && (src_y < SPINNER_SPRITE_H))
      {
        if (s_hourglass_sprite[src_y][src_x] != 0U)
        {
          Spinner_Drv_SetPixel4bpp(frame,
                                   (uint32_t)((int32_t)SPINNER_CENTER_X + dx),
                                   (uint32_t)((int32_t)SPINNER_CENTER_Y + dy),
                                   0x0FU);
        }
      }
    }
  }
}

/* 5x7 pixel font for digits 0-9 (better visibility) */
static const uint8_t s_digit_font[10][7] = {
  {0x1F,0x11,0x11,0x11,0x11,0x11,0x1F}, /* 0 */
  {0x04,0x0C,0x04,0x04,0x04,0x04,0x1F}, /* 1 */
  {0x1F,0x01,0x01,0x1F,0x10,0x10,0x1F}, /* 2 */
  {0x1F,0x01,0x01,0x1F,0x01,0x01,0x1F}, /* 3 */
  {0x11,0x11,0x11,0x1F,0x01,0x01,0x01}, /* 4 */
  {0x1F,0x10,0x10,0x1F,0x01,0x01,0x1F}, /* 5 */
  {0x1F,0x10,0x10,0x1F,0x11,0x11,0x1F}, /* 6 */
  {0x1F,0x01,0x01,0x02,0x04,0x08,0x10}, /* 7 */
  {0x1F,0x11,0x11,0x1F,0x11,0x11,0x1F}, /* 8 */
  {0x1F,0x11,0x11,0x1F,0x01,0x01,0x1F}  /* 9 */
};

void Spinner_Drv_DrawNumber(uint8_t *frame, uint32_t x, uint32_t y, uint32_t num)
{
  uint8_t digits[5];
  uint8_t digit_count = 0;
  uint32_t temp = num;
  uint32_t i, row, col;
  uint8_t pattern;
  
  /* Handle zero case */
  if (temp == 0U)
  {
    digits[digit_count++] = 0U;
  }
  else
  {
    /* Extract digits */
    while (temp > 0U && digit_count < 5U)
    {
      digits[digit_count++] = (uint8_t)(temp % 10U);
      temp /= 10U;
    }
  }
  
  /* Draw digits (reverse order for correct display) */
  for (i = 0; i < digit_count; i++)
  {
    uint8_t digit = digits[digit_count - 1U - i];
    uint32_t digit_x = x + (i * 6U); /* 6 pixels spacing (5 + 1 gap) */
    
    for (row = 0; row < 7U; row++)
    {
      pattern = s_digit_font[digit][row];
      for (col = 0; col < 5U; col++)
      {
        if ((pattern & (1U << (4U - col))) != 0U)
        {
          Spinner_Drv_SetPixel4bpp(frame, digit_x + col, y + row, 0x0FU);
        }
      }
    }
  }
}

/* Simple character map for debug text */
static const uint8_t s_char_font[26][7] = {
  {0x1F,0x11,0x11,0x1F,0x11,0x11,0x11}, /* A */
  {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, /* B */
  {0x1F,0x10,0x10,0x10,0x10,0x10,0x1F}, /* C */
  {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, /* D */
  {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, /* E */
  {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, /* F */
  {0x1F,0x10,0x10,0x17,0x11,0x11,0x1F}, /* G */
  {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, /* H */
  {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}, /* I */
  {0x0F,0x02,0x02,0x02,0x02,0x12,0x0C}, /* J */
  {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, /* K */
  {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, /* L */
  {0x11,0x1B,0x15,0x11,0x11,0x11,0x11}, /* M */
  {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, /* N */
  {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, /* O */
  {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, /* P */
  {0x0E,0x11,0x11,0x11,0x13,0x11,0x0F}, /* Q */
  {0x1E,0x11,0x11,0x1E,0x12,0x11,0x11}, /* R */
  {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, /* S */
  {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, /* T */
  {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, /* U */
  {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04}, /* V */
  {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, /* W */
  {0x11,0x0A,0x0A,0x04,0x0A,0x0A,0x11}, /* X */
  {0x11,0x0A,0x0A,0x04,0x04,0x04,0x04}, /* Y */
  {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}  /* Z */
};

void Spinner_Drv_DrawText(uint8_t *frame, uint32_t x, uint32_t y, const char *text)
{
  uint32_t i, row, col;
  uint8_t pattern;
  uint32_t current_x = x;
  
  if (text == NULL)
  {
    return;
  }
  
  for (i = 0; text[i] != '\0'; i++)
  {
    char c = text[i];
    uint32_t char_idx;
    
    if (c >= 'A' && c <= 'Z')
    {
      char_idx = (uint32_t)(c - 'A');
    }
    else if (c >= 'a' && c <= 'z')
    {
      char_idx = (uint32_t)(c - 'a');
    }
    else
    {
      current_x += 6U; /* Skip unknown characters */
      continue;
    }
    
    /* Draw character */
    for (row = 0; row < 7U; row++)
    {
      pattern = s_char_font[char_idx][row];
      for (col = 0; col < 5U; col++)
      {
        if ((pattern & (1U << (4U - col))) != 0U)
        {
          Spinner_Drv_SetPixel4bpp(frame, current_x + col, y + row, 0x0FU);
        }
      }
    }
    
    current_x += 6U; /* 6 pixels per character (5 + 1 gap) */
  }
}


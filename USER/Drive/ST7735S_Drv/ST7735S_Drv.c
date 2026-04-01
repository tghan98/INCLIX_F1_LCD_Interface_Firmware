#include "ST7735S_Drv.h"
#include "string.h"

#define ST7735S_PANEL_WIDTH           128U
#define ST7735S_PANEL_HEIGHT          96U
#define ST7735S_RAM_OFFSET_X          0U
#define ST7735S_RAM_OFFSET_Y          32U
#define ST7735S_FRAME_OFFSET_X        0U
#define ST7735S_FRAME_OFFSET_Y        17U
#define ST7735S_MADCTL_DEFAULT        0x08U
#define ST7735S_COLMOD_RGB565         0x05U
#define ST7735S_TUNING_PATTERN_MODE   0U
#define ST7735S_DEBUG_RECT_ENABLE     1U
#define ST7735S_DEBUG_RECT_COLOR      0xFFFFU
#define ST7735S_DEBUG_RECT_X0         23U
#define ST7735S_DEBUG_RECT_Y0         2U
#define ST7735S_DEBUG_RECT_X1         107U
#define ST7735S_DEBUG_RECT_Y1         95U
#define LCD_BL_ACTIVE_HIGH            1U
#define LCD_BL_DEFAULT_PERCENT        90U

extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim14;

static void st7735s_draw_debug_rect(void);

static void lcd_select(uint8_t selected)
{
  GPIO_PinState cs_state;

  if (selected != 0U)
  {
    cs_state = GPIO_PIN_RESET;
  }
  else
  {
    cs_state = GPIO_PIN_SET;
  }

  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, cs_state);
}

static void lcd_write_cmd(uint8_t cmd)
{
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
  lcd_select(1U);
  (void)HAL_SPI_Transmit(&hspi2, &cmd, 1U, HAL_MAX_DELAY);
  lcd_select(0U);
}

static void lcd_write_data(const uint8_t *data, uint16_t len)
{
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  lcd_select(1U);
  (void)HAL_SPI_Transmit(&hspi2, (uint8_t *)data, len, HAL_MAX_DELAY);
  lcd_select(0U);
}

static void lcd_write_data8(uint8_t data)
{
  lcd_write_data(&data, 1U);
}

static void lcd_backlight_set_percent(uint8_t percent)
{
  uint32_t pulse;
  uint32_t period;

  if (percent > 100U)
  {
    percent = 100U;
  }

  period = __HAL_TIM_GET_AUTORELOAD(&htim14);
  pulse = ((period + 1U) * percent) / 100U;
  if (pulse > period)
  {
    pulse = period;
  }

#if (LCD_BL_ACTIVE_HIGH != 0U)
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, pulse);
#else
  __HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, period - pulse);
#endif
}

static void lcd_backlight_init(void)
{
  (void)HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
  lcd_backlight_set_percent(LCD_BL_DEFAULT_PERCENT);
}

static void lcd_write_cmd_with_data(uint8_t cmd, const uint8_t *data, uint16_t len)
{
  lcd_write_cmd(cmd);
  if ((data != NULL) && (len > 0U))
  {
    lcd_write_data(data, len);
  }
}

static void lcd_hard_reset(void)
{
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(10U);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(120U);
}

static void st7735s_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
  uint8_t data[4];

  lcd_write_cmd(0x2AU); /* CASET */
  data[0] = (uint8_t)(x_start >> 8);
  data[1] = (uint8_t)(x_start & 0xFFU);
  data[2] = (uint8_t)(x_end >> 8);
  data[3] = (uint8_t)(x_end & 0xFFU);
  lcd_write_data(data, (uint16_t)sizeof(data));

  lcd_write_cmd(0x2BU); /* RASET */
  data[0] = (uint8_t)(y_start >> 8);
  data[1] = (uint8_t)(y_start & 0xFFU);
  data[2] = (uint8_t)(y_end >> 8);
  data[3] = (uint8_t)(y_end & 0xFFU);
  lcd_write_data(data, (uint16_t)sizeof(data));
}

static void st7735s_clear_black(void)
{
  uint8_t line[(uint16_t)(ST7735S_PANEL_WIDTH * 2U)];
  uint16_t y;

  memset(line, 0, sizeof(line));

  st7735s_set_window((uint16_t)ST7735S_RAM_OFFSET_X,
                     (uint16_t)ST7735S_RAM_OFFSET_Y,
                     (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_PANEL_WIDTH - 1U),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_PANEL_HEIGHT - 1U));
  lcd_write_cmd(0x2CU); /* RAMWR */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  lcd_select(1U);
  for (y = 0U; y < ST7735S_PANEL_HEIGHT; y++)
  {
    (void)HAL_SPI_Transmit(&hspi2, line, (uint16_t)sizeof(line), HAL_MAX_DELAY);
  }
  lcd_select(0U);
}

static uint16_t st7735s_gray_to_565(uint8_t gray4)
{
  uint8_t v8;
  uint8_t r5;
  uint8_t g6;
  uint8_t b5;

  v8 = (uint8_t)((gray4 << 4) | gray4);
  r5 = (uint8_t)(v8 >> 3);
  g6 = (uint8_t)(v8 >> 2);
  b5 = (uint8_t)(v8 >> 3);

  return (uint16_t)(((uint16_t)r5 << 11) | ((uint16_t)g6 << 5) | (uint16_t)b5);
}

static void st7735s_draw_tuning_pattern(void)
{
  uint8_t tx_line[(uint16_t)(ST7735S_PANEL_WIDTH * 2U)];
  uint16_t y;
  uint16_t x;
  uint16_t color565;

  st7735s_set_window((uint16_t)ST7735S_RAM_OFFSET_X,
                     (uint16_t)ST7735S_RAM_OFFSET_Y,
                     (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_PANEL_WIDTH - 1U),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_PANEL_HEIGHT - 1U));
  lcd_write_cmd(0x2CU); /* RAMWR */

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  lcd_select(1U);

  /* Phase-1 debug: force full solid color only. */
  color565 = st7735s_gray_to_565(15U);
  for (x = 0U; x < ST7735S_PANEL_WIDTH; x++)
  {
    tx_line[(x * 2U)] = (uint8_t)(color565 >> 8);
    tx_line[(x * 2U) + 1U] = (uint8_t)(color565 & 0xFFU);
  }

  for (y = 0U; y < ST7735S_PANEL_HEIGHT; y++)
  {
    (void)HAL_SPI_Transmit(&hspi2, tx_line, (uint16_t)sizeof(tx_line), HAL_MAX_DELAY);
  }

  lcd_select(0U);
}

void ST7735S_Drv_Init(void)
{
  static const uint8_t frmctr1[] = {0x01U, 0x2CU, 0x2DU};
  static const uint8_t frmctr2[] = {0x01U, 0x2CU, 0x2DU};
  static const uint8_t frmctr3[] = {0x01U, 0x2CU, 0x2DU, 0x01U, 0x2CU, 0x2DU};
  static const uint8_t invctr[] = {0x07U};
  static const uint8_t pwctr1[] = {0xA2U, 0x02U, 0x84U};
  static const uint8_t pwctr2[] = {0xC5U};
  static const uint8_t pwctr3[] = {0x0AU, 0x00U};
  static const uint8_t pwctr4[] = {0x8AU, 0x2AU};
  static const uint8_t pwctr5[] = {0x8AU, 0xEEU};
  static const uint8_t vmctr1[] = {0x0EU};

  lcd_backlight_init();
  HAL_Delay(20U);

  lcd_hard_reset();

  lcd_write_cmd(0x01U); /* SWRESET */
  HAL_Delay(120U);

  lcd_write_cmd_with_data(0xB1U, frmctr1, (uint16_t)sizeof(frmctr1)); /* FRMCTR1 */
  lcd_write_cmd_with_data(0xB2U, frmctr2, (uint16_t)sizeof(frmctr2)); /* FRMCTR2 */
  lcd_write_cmd_with_data(0xB3U, frmctr3, (uint16_t)sizeof(frmctr3)); /* FRMCTR3 */
  lcd_write_cmd_with_data(0xB4U, invctr, (uint16_t)sizeof(invctr));   /* INVCTR */
  lcd_write_cmd_with_data(0xC0U, pwctr1, (uint16_t)sizeof(pwctr1));   /* PWCTR1 */
  lcd_write_cmd_with_data(0xC1U, pwctr2, (uint16_t)sizeof(pwctr2));   /* PWCTR2 */
  lcd_write_cmd_with_data(0xC2U, pwctr3, (uint16_t)sizeof(pwctr3));   /* PWCTR3 */
  lcd_write_cmd_with_data(0xC3U, pwctr4, (uint16_t)sizeof(pwctr4));   /* PWCTR4 */
  lcd_write_cmd_with_data(0xC4U, pwctr5, (uint16_t)sizeof(pwctr5));   /* PWCTR5 */
  lcd_write_cmd_with_data(0xC5U, vmctr1, (uint16_t)sizeof(vmctr1));   /* VMCTR1 */

  lcd_write_cmd(0x20U); /* INVOFF */

  lcd_write_cmd(0x11U); /* SLPOUT */
  HAL_Delay(120U);

  lcd_write_cmd(0x3AU); /* COLMOD */
  lcd_write_data8(ST7735S_COLMOD_RGB565);

  lcd_write_cmd(0x36U); /* MADCTL */
  lcd_write_data8(ST7735S_MADCTL_DEFAULT);

  lcd_write_cmd(0x13U); /* NORON */
  HAL_Delay(10U);

  lcd_write_cmd(0x29U); /* DISPON */
  HAL_Delay(50U);

  st7735s_clear_black();
#if (ST7735S_TUNING_PATTERN_MODE != 0U)
  st7735s_draw_tuning_pattern();
#endif
  st7735s_draw_debug_rect();
}

void ST7735S_Drv_WriteFrame(const uint8_t *frame)
{
#if (ST7735S_TUNING_PATTERN_MODE != 0U)
  (void)frame;
  st7735s_draw_tuning_pattern();
  st7735s_draw_debug_rect();
  return;
#else
  static const uint16_t gray_lut_565[16] =
  {
    0x0000U, 0x1082U, 0x2104U, 0x3186U,
    0x4208U, 0x52AAU, 0x632CU, 0x73AEU,
    0x8C51U, 0x9CD3U, 0xAD55U, 0xBDD7U,
    0xCE79U, 0xDEFBU, 0xEF7DU, 0xFFFFU
  };
  uint8_t tx_line[(uint16_t)(ST7735S_PANEL_WIDTH * 2U)];
  uint32_t y;
  uint32_t x;

  if (frame == NULL)
  {
    return;
  }

  st7735s_set_window((uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_FRAME_OFFSET_X),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_FRAME_OFFSET_Y),
                     (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_FRAME_OFFSET_X + ST7735S_PANEL_WIDTH - 1U),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_FRAME_OFFSET_Y + ST7735S_DRV_HEIGHT - 1U));
  lcd_write_cmd(0x2CU); /* RAMWR */

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  lcd_select(1U);
  for (y = 0U; y < ST7735S_DRV_HEIGHT; y++)
  {
    const uint8_t *src_line;
    uint32_t src_idx;
    uint32_t dst_idx;

    src_line = &frame[y * (ST7735S_DRV_WIDTH / 2U)];
    src_idx = 0U;
    dst_idx = 0U;

    for (x = 0U; x < ST7735S_PANEL_WIDTH; x++)
    {
      uint8_t packed;
      uint8_t gray_left;
      uint8_t gray_right;
      uint8_t gray_mid;
      uint16_t px;

      packed = src_line[src_idx++];
      gray_left = (uint8_t)((packed >> 4) & 0x0FU);
      gray_right = (uint8_t)(packed & 0x0FU);
      gray_mid = (uint8_t)((gray_left + gray_right) >> 1);

      px = gray_lut_565[gray_mid];

      tx_line[dst_idx++] = (uint8_t)(px >> 8);
      tx_line[dst_idx++] = (uint8_t)(px & 0xFFU);
    }

    (void)HAL_SPI_Transmit(&hspi2, tx_line, (uint16_t)sizeof(tx_line), HAL_MAX_DELAY);
  }
  lcd_select(0U);
  st7735s_draw_debug_rect();
#endif
}

static void st7735s_draw_pixel(uint16_t x, uint16_t y, uint16_t rgb565)
{
  uint8_t px[2];

  if ((x >= ST7735S_PANEL_WIDTH) || (y >= ST7735S_PANEL_HEIGHT))
  {
    return;
  }

  /* Hard clip to measured visible area (bezel-safe bounds) */
  if ((x < ST7735S_VIEW_X_MIN) || (x > ST7735S_VIEW_X_MAX) ||
      (y < ST7735S_VIEW_Y_MIN) || (y > ST7735S_VIEW_Y_MAX))
  {
    return;
  }

  st7735s_set_window((uint16_t)(ST7735S_RAM_OFFSET_X + x),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + y),
                     (uint16_t)(ST7735S_RAM_OFFSET_X + x),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + y));
  lcd_write_cmd(0x2CU); /* RAMWR */

  px[0] = (uint8_t)(rgb565 >> 8);
  px[1] = (uint8_t)(rgb565 & 0xFFU);
  lcd_write_data(px, 2U);
}

static void st7735s_draw_debug_rect(void)
{
#if (ST7735S_DEBUG_RECT_ENABLE != 0U)
  uint16_t x;
  uint16_t y;

  /* 상단선: (X0,Y0) -> (X1,Y0) */
  for (x = ST7735S_DEBUG_RECT_X0; x <= ST7735S_DEBUG_RECT_X1; x++)
  {
    st7735s_draw_pixel(x, ST7735S_DEBUG_RECT_Y0, ST7735S_DEBUG_RECT_COLOR);
  }

  /* 하단선: (X0,Y1) -> (X1,Y1) */
  for (x = ST7735S_DEBUG_RECT_X0; x <= ST7735S_DEBUG_RECT_X1; x++)
  {
    st7735s_draw_pixel(x, ST7735S_DEBUG_RECT_Y1, ST7735S_DEBUG_RECT_COLOR);
  }

  /* 좌측 세로선: (X0,Y0) -> (X0,Y1) */
  for (y = ST7735S_DEBUG_RECT_Y0; y <= ST7735S_DEBUG_RECT_Y1; y++)
  {
    st7735s_draw_pixel(ST7735S_DEBUG_RECT_X0, y, ST7735S_DEBUG_RECT_COLOR);
  }

  /* 우측 세로선: (X1,Y0) -> (X1,Y1) */
  for (y = ST7735S_DEBUG_RECT_Y0; y <= ST7735S_DEBUG_RECT_Y1; y++)
  {
    st7735s_draw_pixel(ST7735S_DEBUG_RECT_X1, y, ST7735S_DEBUG_RECT_COLOR);
  }
#endif
}

static const uint8_t *st7735s_get_glyph_3x5(char ch)
{
  static const uint8_t glyph_space[3] = {0x00, 0x00, 0x00};
  static const uint8_t glyph_colon[3] = {0x00, 0x0AU, 0x00};
  static const uint8_t glyph_slash[3] = {0x10, 0x08, 0x04};
  static const uint8_t glyph_gt[3] = {0x04, 0x0AU, 0x11};
  static const uint8_t glyph_0[3] = {0x1FU, 0x11U, 0x1FU};
  static const uint8_t glyph_1[3] = {0x00U, 0x1FU, 0x00U};
  static const uint8_t glyph_2[3] = {0x1DU, 0x15U, 0x17U};
  static const uint8_t glyph_3[3] = {0x15U, 0x15U, 0x1FU};
  static const uint8_t glyph_4[3] = {0x07U, 0x04U, 0x1FU};
  static const uint8_t glyph_5[3] = {0x17U, 0x15U, 0x1DU};
  static const uint8_t glyph_6[3] = {0x1FU, 0x15U, 0x1DU};
  static const uint8_t glyph_7[3] = {0x01U, 0x01U, 0x1FU};
  static const uint8_t glyph_8[3] = {0x1FU, 0x15U, 0x1FU};
  static const uint8_t glyph_9[3] = {0x17U, 0x15U, 0x1FU};
  static const uint8_t glyph_A[3] = {0x1FU, 0x05U, 0x1FU};
  static const uint8_t glyph_B[3] = {0x1FU, 0x15U, 0x0AU};
  static const uint8_t glyph_C[3] = {0x1FU, 0x11U, 0x11U};
  static const uint8_t glyph_D[3] = {0x1FU, 0x11U, 0x0EU};
  static const uint8_t glyph_E[3] = {0x1FU, 0x15U, 0x11U};
  static const uint8_t glyph_F[3] = {0x1FU, 0x05U, 0x01U};
  static const uint8_t glyph_G[3] = {0x1FU, 0x11U, 0x1DU};
  static const uint8_t glyph_H[3] = {0x1FU, 0x04U, 0x1FU};
  static const uint8_t glyph_I[3] = {0x11U, 0x1FU, 0x11U};
  static const uint8_t glyph_K[3] = {0x1FU, 0x04U, 0x1BU};
  static const uint8_t glyph_L[3] = {0x1FU, 0x10U, 0x10U};
  static const uint8_t glyph_M[3] = {0x1FU, 0x02U, 0x1FU};
  static const uint8_t glyph_N[3] = {0x1FU, 0x01U, 0x1EU};
  static const uint8_t glyph_O[3] = {0x1FU, 0x11U, 0x1FU};
  static const uint8_t glyph_P[3] = {0x1FU, 0x05U, 0x07U};
  static const uint8_t glyph_R[3] = {0x1FU, 0x0DU, 0x17U};
  static const uint8_t glyph_S[3] = {0x17U, 0x15U, 0x1DU};
  static const uint8_t glyph_T[3] = {0x01U, 0x1FU, 0x01U};
  static const uint8_t glyph_U[3] = {0x1FU, 0x10U, 0x1FU};
  static const uint8_t glyph_V[3] = {0x0FU, 0x10U, 0x0FU};
  static const uint8_t glyph_W[3] = {0x1FU, 0x08U, 0x1FU};
  static const uint8_t glyph_X[3] = {0x1BU, 0x04U, 0x1BU};
  static const uint8_t glyph_Y[3] = {0x03U, 0x1CU, 0x03U};

  switch (ch)
  {
    case ' ': return glyph_space;
    case ':': return glyph_colon;
    case '/': return glyph_slash;
    case '>': return glyph_gt;
    case '0': return glyph_0;
    case '1': return glyph_1;
    case '2': return glyph_2;
    case '3': return glyph_3;
    case '4': return glyph_4;
    case '5': return glyph_5;
    case '6': return glyph_6;
    case '7': return glyph_7;
    case '8': return glyph_8;
    case '9': return glyph_9;
    case 'A': return glyph_A;
    case 'B': return glyph_B;
    case 'C': return glyph_C;
    case 'D': return glyph_D;
    case 'E': return glyph_E;
    case 'F': return glyph_F;
    case 'G': return glyph_G;
    case 'H': return glyph_H;
    case 'I': return glyph_I;
    case 'K': return glyph_K;
    case 'L': return glyph_L;
    case 'M': return glyph_M;
    case 'N': return glyph_N;
    case 'O': return glyph_O;
    case 'P': return glyph_P;
    case 'R': return glyph_R;
    case 'S': return glyph_S;
    case 'T': return glyph_T;
    case 'U': return glyph_U;
    case 'V': return glyph_V;
    case 'W': return glyph_W;
    case 'X': return glyph_X;
    case 'Y': return glyph_Y;
    default:  return glyph_space;
  }
}

void ST7735S_Drv_DrawChar3x5(uint16_t x, uint16_t y, char ch, uint16_t fg_rgb565, uint16_t bg_rgb565)
{
  const uint8_t *glyph;
  uint16_t row;
  uint16_t col;

  glyph = st7735s_get_glyph_3x5(ch);

  for (row = 0U; row < 5U; row++)
  {
    for (col = 0U; col < 4U; col++)
    {
      uint16_t px = bg_rgb565;

      if ((col < 3U) && ((glyph[col] & (uint8_t)(1U << row)) != 0U))
      {
        px = fg_rgb565;
      }

      st7735s_draw_pixel((uint16_t)(x + col), (uint16_t)(y + row), px);
    }
  }
}

void ST7735S_Drv_DrawString3x5(uint16_t x, uint16_t y, const char *text, uint16_t fg_rgb565, uint16_t bg_rgb565)
{
  uint16_t cursor_x = x;

  if (text == NULL)
  {
    return;
  }

  while (*text != '\0')
  {
    ST7735S_Drv_DrawChar3x5(cursor_x, y, *text, fg_rgb565, bg_rgb565);
    cursor_x = (uint16_t)(cursor_x + 4U);
    text++;
  }
}

void ST7735S_Drv_Clear(uint16_t rgb565)
{
  uint8_t line[(uint16_t)(ST7735S_PANEL_WIDTH * 2U)];
  uint16_t y;
  uint16_t x;

  for (x = 0U; x < ST7735S_PANEL_WIDTH; x++)
  {
    line[2U * x] = (uint8_t)(rgb565 >> 8);
    line[(2U * x) + 1U] = (uint8_t)(rgb565 & 0xFFU);
  }

  st7735s_set_window((uint16_t)ST7735S_RAM_OFFSET_X,
                     (uint16_t)ST7735S_RAM_OFFSET_Y,
                     (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_PANEL_WIDTH - 1U),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_PANEL_HEIGHT - 1U));
  lcd_write_cmd(0x2CU); /* RAMWR */

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  lcd_select(1U);
  for (y = 0U; y < ST7735S_PANEL_HEIGHT; y++)
  {
    (void)HAL_SPI_Transmit(&hspi2, line, (uint16_t)sizeof(line), HAL_MAX_DELAY);
  }
  lcd_select(0U);
  st7735s_draw_debug_rect();
}

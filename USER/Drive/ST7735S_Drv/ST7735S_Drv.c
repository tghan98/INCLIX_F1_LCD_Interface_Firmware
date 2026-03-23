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
#define LCD_BL_ACTIVE_HIGH            1U
#define LCD_BL_DEFAULT_PERCENT        90U

extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim14;

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
}

void ST7735S_Drv_WriteFrame(const uint8_t *frame)
{
#if (ST7735S_TUNING_PATTERN_MODE != 0U)
  (void)frame;
  st7735s_draw_tuning_pattern();
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
#endif
}

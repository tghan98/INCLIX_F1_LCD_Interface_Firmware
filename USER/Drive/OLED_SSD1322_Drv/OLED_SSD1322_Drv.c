#include "OLED_SSD1322_Drv.h"

#define OLED_POWER_ON_LEVEL      GPIO_PIN_RESET
#define OLED_COL_START           0x1CU
#define OLED_COL_END             0x5BU
#define OLED_ROW_START           0x00U
#define OLED_ROW_END             0x3FU

extern SPI_HandleTypeDef hspi2;

static void oled_select(uint8_t selected)
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

static void oled_write_cmd(uint8_t cmd)
{
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);
  oled_select(1U);
  (void)HAL_SPI_Transmit(&hspi2, &cmd, 1U, HAL_MAX_DELAY);
  oled_select(0U);
}

static void oled_write_data(const uint8_t *data, uint16_t len)
{
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  oled_select(1U);
  (void)HAL_SPI_Transmit(&hspi2, (uint8_t *)data, len, HAL_MAX_DELAY);
  oled_select(0U);
}

static void oled_write_data8(uint8_t data)
{
  oled_write_data(&data, 1U);
}

static void oled_hard_reset(void)
{
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
  HAL_Delay(10U);
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
  HAL_Delay(10U);
}

static void oled_set_window_full(void)
{
  oled_write_cmd(0x15U);
  oled_write_data8(OLED_COL_START);
  oled_write_data8(OLED_COL_END);

  oled_write_cmd(0x75U);
  oled_write_data8(OLED_ROW_START);
  oled_write_data8(OLED_ROW_END);
}

void OLED_SSD1322_Drv_Init(void)
{
  static const uint8_t remap[] = {0x14U, 0x11U};
  static const uint8_t enhance_a[] = {0xA0U, 0xFDU};
  static const uint8_t enhance_b[] = {0xA2U, 0x20U};

  HAL_GPIO_WritePin(LCDVCC_EN_GPIO_Port, LCDVCC_EN_Pin, OLED_POWER_ON_LEVEL);
  HAL_Delay(50U);

  oled_hard_reset();
  HAL_Delay(20U);

  oled_write_cmd(0xFDU);
  oled_write_data8(0x12U);

  oled_write_cmd(0xAEU);

  oled_write_cmd(0xB3U);
  oled_write_data8(0x91U);

  oled_write_cmd(0xCAU);
  oled_write_data8(0x3FU);

  oled_write_cmd(0xA2U);
  oled_write_data8(0x00U);

  oled_write_cmd(0xA1U);
  oled_write_data8(0x00U);

  oled_write_cmd(0xA0U);
  oled_write_data(remap, (uint16_t)sizeof(remap));

  oled_write_cmd(0xABU);
  oled_write_data8(0x01U);

  oled_write_cmd(0xB4U);
  oled_write_data(enhance_a, (uint16_t)sizeof(enhance_a));

  oled_write_cmd(0xC1U);
  oled_write_data8(0x9FU);

  oled_write_cmd(0xC7U);
  oled_write_data8(0x0FU);

  oled_write_cmd(0xB1U);
  oled_write_data8(0xE2U);

  oled_write_cmd(0xD1U);
  oled_write_data(enhance_b, (uint16_t)sizeof(enhance_b));

  oled_write_cmd(0xBBU);
  oled_write_data8(0x1FU);

  oled_write_cmd(0xB6U);
  oled_write_data8(0x08U);

  oled_write_cmd(0xBEU);
  oled_write_data8(0x07U);

  oled_write_cmd(0xA6U);
  oled_write_cmd(0xAFU);
  HAL_Delay(20U);
}

void OLED_SSD1322_Drv_WriteFrame(const uint8_t *frame)
{
  oled_set_window_full();
  oled_write_cmd(0x5CU);

  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
  oled_select(1U);
  (void)HAL_SPI_Transmit(&hspi2, (uint8_t *)frame, OLED_SSD1322_FRAME_BYTES, HAL_MAX_DELAY);
  oled_select(0U);
}

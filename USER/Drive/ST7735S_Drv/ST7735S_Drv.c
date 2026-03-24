#include "ST7735S_Drv.h"

#define ST7735S_RAM_OFFSET_X    2U
#define ST7735S_RAM_OFFSET_Y    32U
#define ST7735S_RAM_ROWS        160U
#define ST7735S_MADCTL_DEFAULT  0x08U
#define ST7735S_COLMOD_RGB565   0x05U

extern SPI_HandleTypeDef hspi2;

static void lcd_select(uint8_t selected)
{
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, (selected != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
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

static void st7735s_draw_pixel(uint16_t x, uint16_t y, uint16_t rgb565)
{
	uint8_t px[2];

	if ((x >= ST7735S_PANEL_WIDTH) || (y >= ST7735S_PANEL_HEIGHT))
	{
		return;
	}

	/* Hard clip to measured visible area so rendering never leaks outside bezel-safe bounds. */
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

static void st7735s_clear_window_black(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	uint8_t line[(uint16_t)(ST7735S_PANEL_WIDTH * 2U)] = {0};
	uint16_t row;

	st7735s_set_window(x,
										 y,
										 (uint16_t)(x + w - 1U),
										 (uint16_t)(y + h - 1U));
	lcd_write_cmd(0x2CU); /* RAMWR */

	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);
	lcd_select(1U);
	for (row = 0U; row < h; row++)
	{
		(void)HAL_SPI_Transmit(&hspi2, line, (uint16_t)(w * 2U), HAL_MAX_DELAY);
	}
	lcd_select(0U);
}

/* Force-clear full controller rows once to remove stale lines outside active window. */
static void st7735s_clear_full_gram_black(void)
{
	st7735s_clear_window_black(0U, 0U, ST7735S_PANEL_WIDTH, ST7735S_RAM_ROWS);
}

static const uint8_t *st7735s_get_glyph_5x7(char ch)
{
	static const uint8_t glyph_space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
	static const uint8_t glyph_colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
	static const uint8_t glyph_slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
	static const uint8_t glyph_0[5] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
	static const uint8_t glyph_1[5] = {0x00, 0x42, 0x7F, 0x40, 0x00};
	static const uint8_t glyph_2[5] = {0x42, 0x61, 0x51, 0x49, 0x46};
	static const uint8_t glyph_3[5] = {0x21, 0x41, 0x45, 0x4B, 0x31};
	static const uint8_t glyph_4[5] = {0x18, 0x14, 0x12, 0x7F, 0x10};
	static const uint8_t glyph_5[5] = {0x27, 0x45, 0x45, 0x45, 0x39};
	static const uint8_t glyph_6[5] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
	static const uint8_t glyph_7[5] = {0x01, 0x71, 0x09, 0x05, 0x03};
	static const uint8_t glyph_8[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
	static const uint8_t glyph_9[5] = {0x06, 0x49, 0x49, 0x29, 0x1E};
	static const uint8_t glyph_A[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E};
	static const uint8_t glyph_B[5] = {0x7F, 0x49, 0x49, 0x49, 0x36};
	static const uint8_t glyph_C[5] = {0x3E, 0x41, 0x41, 0x41, 0x22};
	static const uint8_t glyph_D[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
	static const uint8_t glyph_E[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
	static const uint8_t glyph_F[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
	static const uint8_t glyph_G[5] = {0x3E, 0x41, 0x49, 0x49, 0x7A};
	static const uint8_t glyph_H[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
	static const uint8_t glyph_I[5] = {0x00, 0x41, 0x7F, 0x41, 0x00};
	static const uint8_t glyph_K[5] = {0x7F, 0x08, 0x14, 0x22, 0x41};
	static const uint8_t glyph_L[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
	static const uint8_t glyph_M[5] = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
	static const uint8_t glyph_N[5] = {0x7F, 0x04, 0x08, 0x10, 0x7F};
	static const uint8_t glyph_O[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
	static const uint8_t glyph_P[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
	static const uint8_t glyph_R[5] = {0x7F, 0x09, 0x19, 0x29, 0x46};
	static const uint8_t glyph_S[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
	static const uint8_t glyph_T[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
	static const uint8_t glyph_U[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F};
	static const uint8_t glyph_V[5] = {0x1F, 0x20, 0x40, 0x20, 0x1F};
	static const uint8_t glyph_W[5] = {0x7F, 0x20, 0x18, 0x20, 0x7F};
	static const uint8_t glyph_X[5] = {0x63, 0x14, 0x08, 0x14, 0x63};
	static const uint8_t glyph_Y[5] = {0x03, 0x04, 0x78, 0x04, 0x03};

	switch (ch)
	{
		case ' ': return glyph_space;
		case ':': return glyph_colon;
		case '/': return glyph_slash;
		case '0': return glyph_0;
		case '1': return glyph_1;
		case 'x': return glyph_X;
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

void ST7735S_Drv_Init(void)
{
	uint8_t colmod = ST7735S_COLMOD_RGB565;
	uint8_t madctl = ST7735S_MADCTL_DEFAULT;

	HAL_GPIO_WritePin(LCDVCC_EN_GPIO_Port, LCDVCC_EN_Pin, GPIO_PIN_SET);
	HAL_Delay(120U);

	lcd_hard_reset();

	/* Datasheet command sequence: SWRESET -> SLPOUT -> COLMOD -> MADCTL -> DISPON */
	lcd_write_cmd(0x01U); /* SWRESET */
	HAL_Delay(150U);

	lcd_write_cmd(0x11U); /* SLPOUT */
	HAL_Delay(120U);

	lcd_write_cmd_with_data(0x3AU, &colmod, 1U); /* COLMOD: RGB565 */
	lcd_write_cmd_with_data(0x36U, &madctl, 1U); /* MADCTL */

	lcd_write_cmd(0x29U); /* DISPON */
	HAL_Delay(20U);

	st7735s_clear_full_gram_black();
	ST7735S_Drv_Clear(0x0000U);
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
}

void ST7735S_Drv_WriteFrame(const uint8_t *frame)
{
	(void)frame;
	ST7735S_Drv_Clear(0x0000U);
}

void ST7735S_Drv_DrawChar5x7(uint16_t x, uint16_t y, char ch, uint16_t fg_rgb565, uint16_t bg_rgb565)
{
	const uint8_t *glyph;
	uint16_t row;
	uint16_t col;

	glyph = st7735s_get_glyph_5x7(ch);

	for (row = 0U; row < 7U; row++)
	{
		for (col = 0U; col < 6U; col++)
		{
			uint16_t px = bg_rgb565;

			if ((col < 5U) && ((glyph[col] & (uint8_t)(1U << row)) != 0U))
			{
				px = fg_rgb565;
			}

			st7735s_draw_pixel((uint16_t)(x + col), (uint16_t)(y + row), px);
		}
	}
}

void ST7735S_Drv_DrawString5x7(uint16_t x, uint16_t y, const char *text, uint16_t fg_rgb565, uint16_t bg_rgb565)
{
	uint16_t cursor_x = x;

	if (text == NULL)
	{
		return;
	}

	while (*text != '\0')
	{
		ST7735S_Drv_DrawChar5x7(cursor_x, y, *text, fg_rgb565, bg_rgb565);
		cursor_x = (uint16_t)(cursor_x + 6U);
		text++;
	}
}

void ST7735S_Drv_TestCornerPixels(void)
{
	uint16_t x;
	uint16_t y;

	ST7735S_Drv_Clear(0x0000U);

	/* 기존 선: (21,48) -> (64,48) */
	for (x = 21U; x <= 64U; x++)
	{
		st7735s_draw_pixel(x, 48U, 0xFFFFU);
	}

	/* 반대 방향 선 추가: (64,48) -> */
	for (x = 64U; x <= 105U; x++)
	{
		st7735s_draw_pixel(x, 48U, 0xFFFFU);
	}

	/* 추가 세로선: (21,2) -> (21,95) */
	for (y = 2U; y <= 95U; y++)
	{
		st7735s_draw_pixel(21U, y, 0xFFFFU);
	}

	/* 추가 하단선: (21,95) -> (105,95) */
	for (x = 21U; x <= 105U; x++)
	{
		st7735s_draw_pixel(x, 95U, 0xFFFFU);
	}

	/* 추가 상단선: (21,2) -> (105,2) */
	for (x = 21U; x <= 105U; x++)
	{
		st7735s_draw_pixel(x, 2U, 0xFFFFU);
	}

	/* 추가 우측 세로선: (105,2) -> (105,95) */
	for (y = 2U; y <= 95U; y++)
	{
		st7735s_draw_pixel(105U, y, 0xFFFFU);
	}
}

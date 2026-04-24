#include "ST7735S_Drv.h"
#include "User_HAL_Drv.h"
#include "string.h"

#define ST7735S_PANEL_WIDTH           128U
#define ST7735S_PANEL_HEIGHT          97U
#define ST7735S_RAM_OFFSET_X          0U
#define ST7735S_RAM_OFFSET_Y          32U
#define ST7735S_FRAME_OFFSET_X        0U
#define ST7735S_FRAME_OFFSET_Y        0U
#define ST7735S_MADCTL_DEFAULT        0x08U
#define ST7735S_COLMOD_RGB565         0x05U
#define ST7735S_TUNING_PATTERN_MODE   0U
#define LCD_BL_ACTIVE_HIGH            1U
#define LCD_BL_DEFAULT_PERCENT        90U

static SPI_HandleTypeDef *lcd_get_spi_handle(void)
{
  // 1) HAL 드라이버에서 LCD SPI 핸들을 읽어 반환합니다.
  return (SPI_HandleTypeDef *)Read_LCD_SPI_HalDrive();
}

static TIM_HandleTypeDef *lcd_get_bl_timer_handle(void)
{
  // 1) HAL 드라이버에서 LCD 백라이트 타이머 핸들을 읽어 반환합니다.
  return (TIM_HandleTypeDef *)Read_LCD_BL_Timer_HalDrive();
}

/**
 * @brief  LCD CS 핀을 제어합니다. (Active-Low)
 * @param  selected 1: 선택, 0: 해제
 * @retval 없음
 */
static void lcd_select(uint8_t selected)
{
  // 1) CS(Active-Low) 선택 상태를 하드웨어에 반영합니다.
  HW_LCD_CS_Select((uint32_t)selected);
}

/**
 * @brief  LCD 명령 1바이트를 전송합니다.
 * @param  cmd 전송할 명령 값
 * @retval 없음
 */
static void lcd_write_cmd(uint8_t cmd)
{
  // 1) D/C를 Command 모드로 전환합니다.
  HW_LCD_DC_Set(0U);
  // 2) CS를 활성화한 뒤 명령 1바이트를 전송합니다.
  lcd_select(1U);
  (void)HAL_SPI_Transmit(lcd_get_spi_handle(), &cmd, 1U, HAL_MAX_DELAY);
  // 3) 전송이 끝나면 CS를 비활성화합니다.
  lcd_select(0U);
}

/**
 * @brief  LCD 데이터 버퍼를 전송합니다.
 * @param  data 전송할 데이터 포인터
 * @param  len  전송 바이트 수
 * @retval 없음
 */
static void lcd_write_data(const uint8_t *data, uint16_t len)
{
  // 1) D/C를 Data 모드로 전환합니다.
  HW_LCD_DC_Set(1U);
  // 2) CS를 활성화한 뒤 데이터 버퍼를 전송합니다.
  lcd_select(1U);
  (void)HAL_SPI_Transmit(lcd_get_spi_handle(), (uint8_t *)data, len, HAL_MAX_DELAY);
  // 3) 전송이 끝나면 CS를 비활성화합니다.
  lcd_select(0U);
}

/**
 * @brief  데이터 1바이트를 전송합니다.
 * @param  data 전송할 1바이트 데이터
 * @retval 없음
 */
static void lcd_write_data8(uint8_t data)
{
  // 1) 1바이트 데이터를 공통 데이터 전송 함수로 전달합니다.
  lcd_write_data(&data, 1U);
}

/**
 * @brief  백라이트 밝기를 퍼센트로 설정합니다.
 * @param  percent 0~100
 * @retval 없음
 */
static void lcd_backlight_set_percent(uint8_t percent)
{
  TIM_HandleTypeDef *p_htim;
  uint32_t pulse;
  uint32_t period;

  // 1) 입력 밝기를 0~100% 범위로 제한합니다.
  if (percent > 100U)
  {
    percent = 100U;
  }

  // 2) 백라이트 PWM 타이머 핸들을 가져옵니다.
  p_htim = lcd_get_bl_timer_handle();
  if (p_htim == NULL)
  {
    return;
  }

  // 3) ARR 기준으로 듀티 펄스 폭을 계산합니다.
  period = __HAL_TIM_GET_AUTORELOAD(p_htim);
  pulse = ((period + 1U) * percent) / 100U;
  if (pulse > period)
  {
    pulse = period;
  }

  // 4) Active High/Low 극성에 맞춰 CCR 값을 설정합니다.
#if (LCD_BL_ACTIVE_HIGH != 0U)
  __HAL_TIM_SET_COMPARE(p_htim, TIM_CHANNEL_1, pulse);
#else
  __HAL_TIM_SET_COMPARE(p_htim, TIM_CHANNEL_1, period - pulse);
#endif
}

/**
 * @brief  백라이트 PWM 출력을 시작하고 기본 밝기를 적용합니다.
 * @param  없음
 * @retval 없음
 */
static void lcd_backlight_init(void)
{
  TIM_HandleTypeDef *p_htim;

  // 1) 백라이트 PWM 타이머 핸들을 가져옵니다.
  p_htim = lcd_get_bl_timer_handle();
  if (p_htim == NULL)
  {
    return;
  }

  // 2) PWM 출력을 시작합니다.
  (void)HAL_TIM_PWM_Start(p_htim, TIM_CHANNEL_1);
  // 3) 기본 밝기 퍼센트를 적용합니다.
  lcd_backlight_set_percent(LCD_BL_DEFAULT_PERCENT);
}

/**
 * @brief  명령과 데이터를 연속으로 전송합니다.
 * @param  cmd  명령 값
 * @param  data 데이터 포인터
 * @param  len  데이터 길이
 * @retval 없음
 */
static void lcd_write_cmd_with_data(uint8_t cmd, const uint8_t *data, uint16_t len)
{
  // 1) 명령을 먼저 전송합니다.
  lcd_write_cmd(cmd);
  // 2) 유효한 데이터가 있으면 이어서 데이터를 전송합니다.
  if ((data != NULL) && (len > 0U))
  {
    lcd_write_data(data, len);
  }
}

/**
 * @brief  LCD 하드웨어 리셋 시퀀스를 수행합니다.
 * @param  없음
 * @retval 없음
 */
static void lcd_hard_reset(void)
{
  // 1) Reset 핀을 Low로 내려 하드웨어 리셋을 시작합니다.
  HW_LCD_Reset(0U);
  HAL_Delay(10U);
  // 2) Reset 핀을 High로 복귀시키고 안정화 대기합니다.
  HW_LCD_Reset(1U);
  HAL_Delay(120U);
}

/**
 * @brief  GRAM 쓰기 윈도우(사각형 영역)를 설정합니다.
 * @param  x_start 시작 X
 * @param  y_start 시작 Y
 * @param  x_end   끝 X
 * @param  y_end   끝 Y
 * @retval 없음
 */
static void st7735s_set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
  uint8_t data[4];

  // 1) CASET으로 X 시작/끝 주소를 설정합니다.
  lcd_write_cmd(0x2AU); /* CASET */
  data[0] = (uint8_t)(x_start >> 8);
  data[1] = (uint8_t)(x_start & 0xFFU);
  data[2] = (uint8_t)(x_end >> 8);
  data[3] = (uint8_t)(x_end & 0xFFU);
  lcd_write_data(data, (uint16_t)sizeof(data));

  // 2) RASET으로 Y 시작/끝 주소를 설정합니다.
  lcd_write_cmd(0x2BU); /* RASET */
  data[0] = (uint8_t)(y_start >> 8);
  data[1] = (uint8_t)(y_start & 0xFFU);
  data[2] = (uint8_t)(y_end >> 8);
  data[3] = (uint8_t)(y_end & 0xFFU);
  lcd_write_data(data, (uint16_t)sizeof(data));
}

/**
 * @brief  패널 전체를 검정색으로 클리어합니다.
 * @param  없음
 * @retval 없음
 */
static void st7735s_clear_black(void)
{
  uint8_t line[(uint16_t)(ST7735S_PANEL_WIDTH * 2U)];
  uint16_t y;

  // 1) 검정색(RGB565=0x0000) 한 줄 버퍼를 준비합니다.
  memset(line, 0, sizeof(line));

  // 2) 전체 패널 영역을 쓰기 윈도우로 설정하고 RAM 쓰기를 시작합니다.
  st7735s_set_window((uint16_t)ST7735S_RAM_OFFSET_X,
                     (uint16_t)ST7735S_RAM_OFFSET_Y,
                     (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_PANEL_WIDTH - 1U),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_PANEL_HEIGHT - 1U));
  lcd_write_cmd(0x2CU); /* RAMWR */
  // 3) 한 줄 버퍼를 높이만큼 반복 전송해 전체를 검정으로 채웁니다.
  HW_LCD_DC_Set(1U);
  lcd_select(1U);
  for (y = 0U; y < ST7735S_PANEL_HEIGHT; y++)
  {
    (void)HAL_SPI_Transmit(lcd_get_spi_handle(), line, (uint16_t)sizeof(line), HAL_MAX_DELAY);
  }
  lcd_select(0U);
}

/**
 * @brief  4비트 그레이 값을 RGB565로 변환합니다.
 * @param  gray4 0~15 그레이 값
 * @retval RGB565 색상값
 */
static uint16_t st7735s_gray_to_565(uint8_t gray4)
{
  uint8_t v8;
  uint8_t r5;
  uint8_t g6;
  uint8_t b5;

  // 1) 4비트 그레이를 8비트로 확장합니다.
  v8 = (uint8_t)((gray4 << 4) | gray4);
  // 2) RGB565 포맷에 맞게 R/G/B 비트폭으로 축소합니다.
  r5 = (uint8_t)(v8 >> 3);
  g6 = (uint8_t)(v8 >> 2);
  b5 = (uint8_t)(v8 >> 3);

  // 3) RGB565 16비트 값으로 패킹해 반환합니다.
  return (uint16_t)(((uint16_t)r5 << 11) | ((uint16_t)g6 << 5) | (uint16_t)b5);
}

/**
 * @brief  튜닝 패턴(단색 전체 채우기)을 출력합니다.
 * @param  없음
 * @retval 없음
 */
static void st7735s_draw_tuning_pattern(void)
{
  uint8_t tx_line[(uint16_t)(ST7735S_PANEL_WIDTH * 2U)];
  uint16_t y;
  uint16_t x;
  uint16_t color565;

  // 1) 전체 패널을 쓰기 윈도우로 설정하고 RAM 쓰기를 시작합니다.
  st7735s_set_window((uint16_t)ST7735S_RAM_OFFSET_X,
                     (uint16_t)ST7735S_RAM_OFFSET_Y,
                     (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_PANEL_WIDTH - 1U),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_PANEL_HEIGHT - 1U));
  lcd_write_cmd(0x2CU); /* RAMWR */

  HW_LCD_DC_Set(1U);
  lcd_select(1U);

  // 2) 단색(최대 그레이) 한 줄 픽셀 버퍼를 생성합니다.
  color565 = st7735s_gray_to_565(15U);
  for (x = 0U; x < ST7735S_PANEL_WIDTH; x++)
  {
    tx_line[(x * 2U)] = (uint8_t)(color565 >> 8);
    tx_line[(x * 2U) + 1U] = (uint8_t)(color565 & 0xFFU);
  }

  // 3) 준비한 한 줄 버퍼를 전체 높이만큼 반복 전송합니다.
  for (y = 0U; y < ST7735S_PANEL_HEIGHT; y++)
  {
    (void)HAL_SPI_Transmit(lcd_get_spi_handle(), tx_line, (uint16_t)sizeof(tx_line), HAL_MAX_DELAY);
  }

  lcd_select(0U);
}

/**
 * @brief  LCD 초기화를 수행합니다.
 * @param  없음
 * @retval 없음
 */
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

  // 1) LCD 전원과 백라이트를 활성화하고 전원 안정화 대기합니다.
  HW_LCD_Power_ONnOFF(1U);
  lcd_backlight_init();
  HAL_Delay(20U);

  // 2) 하드웨어 리셋 후 소프트웨어 리셋을 수행합니다.
  lcd_hard_reset();

  lcd_write_cmd(0x01U); /* SWRESET */
  HAL_Delay(120U);

  // 3) 프레임/전원/전압 관련 초기 레지스터를 순서대로 설정합니다.
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

  // 4) 반전/픽셀포맷/메모리 접근 방향을 설정합니다.
  lcd_write_cmd(0x20U); /* INVOFF */

  lcd_write_cmd(0x11U); /* SLPOUT */
  HAL_Delay(120U);

  lcd_write_cmd(0x3AU); /* COLMOD */
  lcd_write_data8(ST7735S_COLMOD_RGB565);

  lcd_write_cmd(0x36U); /* MADCTL */
  lcd_write_data8(ST7735S_MADCTL_DEFAULT);

  // 5) Normal On -> Display On 순으로 표시를 활성화합니다.
  lcd_write_cmd(0x13U); /* NORON */
  HAL_Delay(10U);

  lcd_write_cmd(0x29U); /* DISPON */
  HAL_Delay(50U);

  // 6) 초기 화면을 클리어하고 필요 시 튜닝 패턴을 출력합니다.
  st7735s_clear_black();
#if (ST7735S_TUNING_PATTERN_MODE != 0U)
  st7735s_draw_tuning_pattern();
#endif
}

/**
 * @brief  4bpp 프레임(128x97)을 LCD로 출력합니다.
 * @param  frame 프레임 버퍼 포인터
 * @retval 없음
 */
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

  // 1) 입력 프레임 포인터 유효성을 확인합니다.
  if (frame == NULL)
  {
    return;
  }

  // 2) 프레임 출력 영역을 윈도우로 설정하고 RAM 쓰기를 시작합니다.
  st7735s_set_window((uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_FRAME_OFFSET_X),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_FRAME_OFFSET_Y),
                     (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_FRAME_OFFSET_X + ST7735S_PANEL_WIDTH - 1U),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_FRAME_OFFSET_Y + ST7735S_DRV_HEIGHT - 1U));
  lcd_write_cmd(0x2CU); /* RAMWR */

  // 3) 각 라인을 4bpp -> RGB565로 변환하여 전송 버퍼를 구성합니다.
  HW_LCD_DC_Set(1U);
  lcd_select(1U);
  for (y = 0U; y < ST7735S_DRV_HEIGHT; y++)
  {
    const uint8_t *src_line;
    uint32_t src_x;
    uint32_t dst_idx;

    src_line = &frame[y * (ST7735S_DRV_WIDTH / 2U)];
    src_x = 0U;
    dst_idx = 0U;

    for (x = 0U; x < (ST7735S_DRV_WIDTH / 2U); x++)
    {
      uint8_t packed;
      uint8_t gray_left;
      uint8_t gray_right;
      uint16_t px_left;
      uint16_t px_right;

      packed = src_line[src_x++];
      gray_left = (uint8_t)((packed >> 4) & 0x0FU);
      gray_right = (uint8_t)(packed & 0x0FU);
      px_left = gray_lut_565[gray_left];
      px_right = gray_lut_565[gray_right];

      tx_line[dst_idx++] = (uint8_t)(px_left >> 8);
      tx_line[dst_idx++] = (uint8_t)(px_left & 0xFFU);

      tx_line[dst_idx++] = (uint8_t)(px_right >> 8);
      tx_line[dst_idx++] = (uint8_t)(px_right & 0xFFU);
    }

    // 4) 변환된 한 줄 데이터를 SPI로 전송합니다.
    (void)HAL_SPI_Transmit(lcd_get_spi_handle(), tx_line, (uint16_t)sizeof(tx_line), HAL_MAX_DELAY);
  }
  lcd_select(0U);
#endif
}

/**
 * @brief  논리 좌표 기준으로 1픽셀을 출력합니다.
 * @param  x 논리 X 좌표
 * @param  y 논리 Y 좌표
 * @param  rgb565 픽셀 색상
 * @retval 없음
 */
static void st7735s_draw_pixel(uint16_t x, uint16_t y, uint16_t rgb565)
{
  uint8_t px[2];

  // 1) 논리 좌표가 유효 범위를 벗어나면 즉시 종료합니다.
  if ((x >= (uint16_t)ST7735S_LOGICAL_WIDTH) || (y >= (uint16_t)ST7735S_LOGICAL_HEIGHT))
  {
    return;
  }

  // 2) 논리 좌표를 실제 RAM 좌표로 변환해 1픽셀 윈도우를 설정합니다.
  st7735s_set_window((uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_VIEW_X_MIN + x),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_VIEW_Y_MIN + y),
                     (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_VIEW_X_MIN + x),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_VIEW_Y_MIN + y));
  lcd_write_cmd(0x2CU); /* RAMWR */

  // 3) RGB565 1픽셀 데이터를 상/하위 바이트로 전송합니다.
  px[0] = (uint8_t)(rgb565 >> 8);
  px[1] = (uint8_t)(rgb565 & 0xFFU);
  lcd_write_data(px, 2U);
}

/**
 * @brief  3x5 글꼴 비트맵 포인터를 반환합니다.
 * @param  ch 출력할 문자
 * @retval 글꼴 데이터 포인터
 */
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

  // 1) 입력 문자에 대응하는 3x5 글리프를 선택합니다.
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

/**
 * @brief  3x5 글자 1개를 출력합니다.
 * @param  x 시작 X 좌표
 * @param  y 시작 Y 좌표
 * @param  ch 출력할 문자
 * @param  fg_rgb565 글자색
 * @param  bg_rgb565 배경색
 * @retval 없음
 */
void ST7735S_Drv_DrawChar3x5(uint16_t x, uint16_t y, char ch, uint16_t fg_rgb565, uint16_t bg_rgb565)
{
  const uint8_t *glyph;
  uint8_t cell_buf[4U * 5U * 2U]; /* 40 bytes: 4 cols x 5 rows x 2 bytes/pixel (RGB565) */
  uint16_t buf_idx;
  uint16_t row;
  uint16_t col;
  uint16_t px;

  // 1) 문자 시작 좌표가 유효 범위를 벗어나면 종료합니다.
  if ((x >= (uint16_t)ST7735S_LOGICAL_WIDTH) || (y >= (uint16_t)ST7735S_LOGICAL_HEIGHT))
  {
    return;
  }

  // 2) 출력할 문자 글리프 포인터를 조회합니다.
  glyph = st7735s_get_glyph_3x5(ch);

  // 3) 4x5 셀이 화면 안에 완전히 들어오면 버퍼 일괄 전송 경로를 사용합니다.
  if ((x <= (uint16_t)(ST7735S_LOGICAL_WIDTH - 4U)) &&
      (y <= (uint16_t)(ST7735S_LOGICAL_HEIGHT - 5U)))
  {
    // 4) 글리프/배경 정보를 기반으로 4x5 RGB565 셀 버퍼를 구성합니다.
    buf_idx = 0U;
    for (row = 0U; row < 5U; row++)
    {
      for (col = 0U; col < 4U; col++)
      {
        px = bg_rgb565;
        if ((col < 3U) && ((glyph[col] & (uint8_t)(1U << row)) != 0U))
        {
          px = fg_rgb565;
        }
        cell_buf[buf_idx++] = (uint8_t)(px >> 8);
        cell_buf[buf_idx++] = (uint8_t)(px & 0xFFU);
      }
    }

    // 5) 문자 셀 영역을 window로 잡고 한 번에 전송합니다.
    st7735s_set_window(
      (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_VIEW_X_MIN + x),
      (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_VIEW_Y_MIN + y),
      (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_VIEW_X_MIN + x + 3U),
      (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_VIEW_Y_MIN + y + 4U));
    lcd_write_cmd(0x2CU); /* RAMWR */
    lcd_write_data(cell_buf, (uint16_t)sizeof(cell_buf));
  }
  else
  {
    // 4) 경계에 걸친 경우 픽셀 단위로 클리핑 출력합니다.
    for (row = 0U; row < 5U; row++)
    {
      for (col = 0U; col < 4U; col++)
      {
        px = bg_rgb565;
        if ((col < 3U) && ((glyph[col] & (uint8_t)(1U << row)) != 0U))
        {
          px = fg_rgb565;
        }
        st7735s_draw_pixel((uint16_t)(x + col), (uint16_t)(y + row), px);
      }
    }
  }
}

/**
 * @brief  문자열을 3x5 폰트로 출력합니다.
 * @param  x 시작 X 좌표
 * @param  y 시작 Y 좌표
 * @param  text 출력할 문자열
 * @param  fg_rgb565 글자색
 * @param  bg_rgb565 배경색
 * @retval 없음
 */
void ST7735S_Drv_DrawString3x5(uint16_t x, uint16_t y, const char *text, uint16_t fg_rgb565, uint16_t bg_rgb565)
{
  uint16_t cursor_x = x;

  // 1) 문자열 포인터 유효성을 확인합니다.
  if (text == NULL)
  {
    return;
  }

  // 2) 문자 단위로 출력하고 커서를 4픽셀씩 이동합니다.
  while (*text != '\0')
  {
    ST7735S_Drv_DrawChar3x5(cursor_x, y, *text, fg_rgb565, bg_rgb565);
    cursor_x = (uint16_t)(cursor_x + 4U);
    text++;
  }
}

/**
 * @brief  패널을 지정 색상으로 클리어합니다.
 * @param  rgb565 채울 색상
 * @retval 없음
 */
void ST7735S_Drv_Clear(uint16_t rgb565)
{
  uint8_t line[(uint16_t)(ST7735S_PANEL_WIDTH * 2U)];
  uint16_t y;
  uint16_t x;

  // 1) 지정 색상의 1라인 RGB565 버퍼를 생성합니다.
  for (x = 0U; x < ST7735S_PANEL_WIDTH; x++)
  {
    line[2U * x] = (uint8_t)(rgb565 >> 8);
    line[(2U * x) + 1U] = (uint8_t)(rgb565 & 0xFFU);
  }

  // 2) 전체 패널을 쓰기 윈도우로 설정하고 라인 버퍼를 반복 전송합니다.
  st7735s_set_window((uint16_t)ST7735S_RAM_OFFSET_X,
                     (uint16_t)ST7735S_RAM_OFFSET_Y,
                     (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_PANEL_WIDTH - 1U),
                     (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_PANEL_HEIGHT - 1U));
  lcd_write_cmd(0x2CU); /* RAMWR */

  HW_LCD_DC_Set(1U);
  lcd_select(1U);
  for (y = 0U; y < ST7735S_PANEL_HEIGHT; y++)
  {
    (void)HAL_SPI_Transmit(lcd_get_spi_handle(), line, (uint16_t)sizeof(line), HAL_MAX_DELAY);
  }
  lcd_select(0U);

  // 3) 코너 4점 마커를 출력해 표시 영역 경계를 확인합니다.
  st7735s_draw_pixel(0U, 0U, 0xFFFFU);
  st7735s_draw_pixel((uint16_t)(ST7735S_LOGICAL_WIDTH - 1U), 0U, 0xFFFFU);
  st7735s_draw_pixel(0U, (uint16_t)(ST7735S_LOGICAL_HEIGHT - 1U), 0xFFFFU);
  st7735s_draw_pixel((uint16_t)(ST7735S_LOGICAL_WIDTH - 1U), (uint16_t)(ST7735S_LOGICAL_HEIGHT - 1U), 0xFFFFU);
}

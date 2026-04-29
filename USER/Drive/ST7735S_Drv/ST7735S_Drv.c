#include "ST7735S_Drv.h"
#include "User_HAL_Drv.h"
#include "string.h"

/* PANEL_WIDTH/HEIGHT, RAM_OFFSET_X/Y는 ST7735S_Drv.h 에서 정의. */
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
 * @brief  3x5 글자 1개를 mono framebuffer에 그린다. Does NOT flush.
 * @param  mono_x 시작 X 좌표 (mono dot 단위)
 * @param  mono_y 시작 Y 좌표 (mono dot 단위)
 * @param  ch     출력할 문자
 * @param  on     1=글리프 dot ON, 0=글리프 dot OFF
 * @retval 없음
 * @note   배경 dot은 건드리지 않는다(투명). 화면 초기화는 ClearMonoBuffer()로 별도 수행.
 *         좌표 클램프는 ST7735S_Drv_DrawMonoDot() 내부에서 1회 수행됨.
 */
void ST7735S_Drv_DrawChar3x5(uint16_t mono_x, uint16_t mono_y, char ch, uint8_t on)
{
  const uint8_t *glyph;
  uint16_t row;
  uint16_t col;

  // 1) 출력할 문자 글리프 포인터를 조회한다.
  glyph = st7735s_get_glyph_3x5(ch);

  // 2) 3x5 dot 중 글리프 비트가 켜진 dot만 framebuffer에 반영한다.
  //    범위 체크/클램프는 DrawMonoDot 내부에서 처리한다.
  for (row = 0U; row < 5U; row++)
  {
    for (col = 0U; col < 3U; col++)
    {
      if ((glyph[col] & (uint8_t)(1U << row)) != 0U)
      {
        ST7735S_Drv_DrawMonoDot((uint16_t)(mono_x + col), (uint16_t)(mono_y + row), on);
      }
    }
  }
}

/**
 * @brief  문자열을 3x5 폰트로 mono framebuffer에 그린다. Does NOT flush.
 * @param  mono_x 시작 X 좌표 (mono dot 단위)
 * @param  mono_y 시작 Y 좌표 (mono dot 단위)
 * @param  text   출력할 문자열
 * @param  on     1=글리프 dot ON, 0=글리프 dot OFF
 * @retval 없음
 * @note   글자 간 cursor 전진은 4 mono dot(글리프 3 + 공백 1).
 */
void ST7735S_Drv_DrawString3x5(uint16_t mono_x, uint16_t mono_y, const char *text, uint8_t on)
{
  uint16_t cursor_x = mono_x;

  // 1) 문자열 포인터 유효성을 확인한다.
  if (text == NULL)
  {
    return;
  }

  // 2) 문자 단위로 출력하고 커서를 4 mono dot씩 이동한다.
  while (*text != '\0')
  {
    ST7735S_Drv_DrawChar3x5(cursor_x, mono_y, *text, on);
    cursor_x = (uint16_t)(cursor_x + 4U);
    text++;
  }
}

/* ============================================================
 * Mono 1bpp Framebuffer (Phase 2)
 *   - 외부 노출 없음. 모두 static.
 *   - 좌표계: mono_x ∈ [0, ST7735S_MONO_WIDTH-1], mono_y ∈ [0, ST7735S_MONO_HEIGHT-1].
 *   - 비트 packing: 한 byte는 같은 라인의 가로 인접 mono dot 8개를 담는다.
 *       byte_index = mono_y * (MONO_WIDTH / 8) + (mono_x / 8)
 *       bit_mask   = 0x80 >> (mono_x % 8)   (MSB가 왼쪽 dot)
 *   - 범위를 벗어난 좌표는 set/get 모두 무시(get은 0 반환). 방어용.
 * ============================================================ */

#define ST7735S_MONO_FB_STRIDE_BYTES   (ST7735S_MONO_WIDTH / 8U)   /* 32 byte/line */

static uint8_t s_mono_fb[ST7735S_MONO_FRAME_BYTES];

/* ============================================================
 * Mono Dirty Line Bitmap (Phase 8)
 *   - 1 bit / mono_y line. set = framebuffer 변경됨, flush 대상.
 *   - mono_y 는 컨트롤러 픽셀 ctrl_y 와 1:1 (Phase 0/1 확정).
 *   - byte_index = mono_y / 8, bit_mask = 1 << (mono_y % 8).
 *   - flush 시 요청 범위 내 연속 dirty span 단위로 set_window+RAMWR+송신
 *     를 분할 수행 (RAMWR 커서는 자동 진행 특성상 clean line을 단순 skip할 수 없음).
 *   - 철칙: framebuffer 변경 진입점(`mono_fb_clear`, `mono_fb_set_dot`)이 dirty 마킹 책임.
 * ============================================================ */

#define ST7735S_MONO_DIRTY_BYTES   ((ST7735S_MONO_HEIGHT + 7U) / 8U)   /* 12 byte */

static uint8_t s_mono_dirty[ST7735S_MONO_DIRTY_BYTES];

/* dirty bitmap 헬퍼. mono_y 범위 체크 포함. */
static void mono_dirty_set_all(void)
{
  (void)memset(s_mono_dirty, 0xFFU, sizeof(s_mono_dirty));
}

static void mono_dirty_mark_line(uint16_t mono_y)
{
  if (mono_y >= ST7735S_MONO_HEIGHT)
  {
    return;
  }
  s_mono_dirty[mono_y >> 3] |= (uint8_t)(1U << ((uint32_t)mono_y & 7U));
}

static void mono_dirty_clear_line(uint16_t mono_y)
{
  if (mono_y >= ST7735S_MONO_HEIGHT)
  {
    return;
  }
  s_mono_dirty[mono_y >> 3] &= (uint8_t)(~(uint8_t)(1U << ((uint32_t)mono_y & 7U)));
}

static uint8_t mono_dirty_is_set(uint16_t mono_y)
{
  if (mono_y >= ST7735S_MONO_HEIGHT)
  {
    return 0U;
  }
  return ((s_mono_dirty[mono_y >> 3] & (uint8_t)(1U << ((uint32_t)mono_y & 7U))) != 0U) ? 1U : 0U;
}

/**
 * @brief  framebuffer 전체를 on/off로 채웁니다.
 * @param  on 0이면 모두 OFF(0x00), 0이 아니면 모두 ON(0xFF).
 */
static void mono_fb_clear(uint8_t on)
{
  // 1) on/off 한 byte 패턴을 만들어 한 번에 메모리 채움.
  uint8_t fill = (on != 0U) ? 0xFFU : 0x00U;
  (void)memset(s_mono_fb, fill, sizeof(s_mono_fb));

  // 2) 전 라인 dirty 마킹. (전체 재송신 보장)
  mono_dirty_set_all();
}

/**
 * @brief  framebuffer의 단일 mono dot을 on/off로 설정합니다.
 * @param  mono_x [0, ST7735S_MONO_WIDTH-1]
 * @param  mono_y [0, ST7735S_MONO_HEIGHT-1]
 * @param  on     0이면 OFF, 0이 아니면 ON.
 * @note   범위를 벗어나면 무시합니다. (방어적 처리)
 */
static void mono_fb_set_dot(uint16_t mono_x, uint16_t mono_y, uint8_t on)
{
  uint32_t byte_index;
  uint8_t  bit_mask;

  // 1) 범위 밖 좌표는 무시.
  if ((mono_x >= ST7735S_MONO_WIDTH) || (mono_y >= ST7735S_MONO_HEIGHT))
  {
    return;
  }

  // 2) 좌표 → byte index, bit mask (MSB가 가로 왼쪽).
  byte_index = ((uint32_t)mono_y * ST7735S_MONO_FB_STRIDE_BYTES) + ((uint32_t)mono_x / 8U);
  bit_mask   = (uint8_t)(0x80U >> ((uint32_t)mono_x & 7U));

  // 3) on/off 비트 갱신.
  if (on != 0U)
  {
    s_mono_fb[byte_index] |= bit_mask;
  }
  else
  {
    s_mono_fb[byte_index] &= (uint8_t)(~bit_mask);
  }

  // 4) 해당 mono_y 라인 dirty 마킹. (실제 비트 변화 여부와 무관하게 set — 단순/안전)
  mono_dirty_mark_line(mono_y);
}

/**
 * @brief  framebuffer의 단일 mono dot 상태를 읽어 반환합니다.
 * @param  mono_x [0, ST7735S_MONO_WIDTH-1]
 * @param  mono_y [0, ST7735S_MONO_HEIGHT-1]
 * @retval 1 = ON, 0 = OFF (범위 밖이면 0).
 */
static uint8_t mono_fb_get_dot(uint16_t mono_x, uint16_t mono_y)
{
  uint32_t byte_index;
  uint8_t  bit_mask;

  // 1) 범위 밖 좌표는 OFF로 간주.
  if ((mono_x >= ST7735S_MONO_WIDTH) || (mono_y >= ST7735S_MONO_HEIGHT))
  {
    return 0U;
  }

  // 2) 좌표 → byte index, bit mask.
  byte_index = ((uint32_t)mono_y * ST7735S_MONO_FB_STRIDE_BYTES) + ((uint32_t)mono_x / 8U);
  bit_mask   = (uint8_t)(0x80U >> ((uint32_t)mono_x & 7U));

  // 3) 비트가 set이면 1 반환.
  return ((s_mono_fb[byte_index] & bit_mask) != 0U) ? 1U : 0U;
}

/* ============================================================
 * Mono → RGB565 Sub-pixel Packing (Phase 3)
 *   - ST7735S 컨트롤러 픽셀 1개(=RGB565 1 word)에는
 *     mono dot 3개(가로로 인접)가 매핑된다.
 *   - 채널 매핑 (Phase 0 캘리브레이션 확정):
 *       subpx 0 → B 채널 (RGB565 하위 5bit)
 *       subpx 1 → G 채널 (RGB565 중간 6bit)
 *       subpx 2 → R 채널 (RGB565 상위 5bit)
 *   - 채널 매핑은 이 한 곳(매크로)에서만 정의한다.
 *     변경 시 다른 위치를 함께 수정할 필요 없도록 단일 진실 원본을 유지한다.
 * ============================================================ */

/* RGB565 채널 풀 강도 마스크. (모노 dot이 ON일 때 해당 채널을 최대치로 켠다) */
#define ST7735S_RGB565_R_FULL    ((uint16_t)0xF800U)   /* 상위 5bit */
#define ST7735S_RGB565_G_FULL    ((uint16_t)0x07E0U)   /* 중간 6bit */
#define ST7735S_RGB565_B_FULL    ((uint16_t)0x001FU)   /* 하위 5bit */

/* mono 그룹 내 sub-pixel index → RGB565 풀강도 마스크. (단일 매핑 정의) */
#define ST7735S_SUBPX_TO_CH_MASK(subpx) \
  (((subpx) == 0U) ? ST7735S_RGB565_B_FULL : \
   ((subpx) == 1U) ? ST7735S_RGB565_G_FULL : \
                     ST7735S_RGB565_R_FULL)

/**
 * @brief  가로로 인접한 mono dot 3개의 on/off 상태를 RGB565 한 픽셀로 packing 한다.
 * @param  sub0_on  mono_x % 3 == 0 위치 dot의 on/off (0=OFF, !=0=ON) → B 채널
 * @param  sub1_on  mono_x % 3 == 1 위치 dot의 on/off                 → G 채널
 * @param  sub2_on  mono_x % 3 == 2 위치 dot의 on/off                 → R 채널
 * @retval RGB565 (uint16_t) packing 결과.
 *
 * @note   세 dot 모두 ON 이면 0xFFFF(흰색),
 *         세 dot 모두 OFF 이면 0x0000(검정)이 된다.
 *         단일 채널만 ON이면 그 채널 풀 강도 값이 그대로 반환된다.
 */
static uint16_t mono3_to_rgb565(uint8_t sub0_on, uint8_t sub1_on, uint8_t sub2_on)
{
  uint16_t pixel = 0U;

  // 1) sub-pixel 0 → B 채널.
  if (sub0_on != 0U)
  {
    pixel |= ST7735S_SUBPX_TO_CH_MASK(0U);
  }

  // 2) sub-pixel 1 → G 채널.
  if (sub1_on != 0U)
  {
    pixel |= ST7735S_SUBPX_TO_CH_MASK(1U);
  }

  // 3) sub-pixel 2 → R 채널.
  if (sub2_on != 0U)
  {
    pixel |= ST7735S_SUBPX_TO_CH_MASK(2U);
  }

  return pixel;
}

/* ============================================================
 * Mono 256x96 Public API (Phase 4)
 *   - 좌표계: mono_x ∈ [0, ST7735S_MONO_USABLE_WIDTH-1],
 *             mono_y ∈ [0, ST7735S_MONO_USABLE_HEIGHT-1].
 *   - 4단계 변환:
 *       rel_ctrl_x = mono_x / 3
 *       rel_ctrl_y = mono_y
 *       ram_x      = RAM_OFFSET_X + VIEW_X_MIN + rel_ctrl_x
 *       ram_y      = RAM_OFFSET_Y + VIEW_Y_MIN + rel_ctrl_y
 *   - 범위 클램프는 mono 좌표 진입 시점에서 1회만 수행한다(이중 적용 금지).
 * ============================================================ */

/* 한 컨트롤러 line 의 RGB565 송신 바이트 수.
 *   컨트롤러 픽셀 수 = ST7735S_LOGICAL_WIDTH (=86), 픽셀당 2 byte → 172 byte. */
#define ST7735S_MONO_LINE_TX_BYTES   ((uint16_t)(ST7735S_LOGICAL_WIDTH * 2U))

/**
 * @brief  framebuffer 1 line 을 RGB565 송신 바이트열로 펼친다.
 * @param  mono_y   원본 라인 (mono 좌표).
 * @param  ctrl_x_start  시작 컨트롤러 픽셀 인덱스(상대, 0..ST7735S_LOGICAL_WIDTH-1).
 * @param  ctrl_x_count  송신할 컨트롤러 픽셀 수.
 * @param  out_buf  출력 버퍼 (크기 >= ctrl_x_count*2).
 *
 * 한 컨트롤러 픽셀 = mono dot 3개 → mono3_to_rgb565()로 packing.
 * mono_x = ctrl_x*3 + (0,1,2). 범위 밖 mono_x는 mono_fb_get_dot()이 0 반환 → 무해.
 */
static void mono_pack_line_to_tx(uint16_t mono_y,
                                 uint16_t ctrl_x_start,
                                 uint16_t ctrl_x_count,
                                 uint8_t  *out_buf)
{
  uint16_t i;
  uint16_t ctrl_x;
  uint16_t mono_x_base;
  uint8_t  s0;
  uint8_t  s1;
  uint8_t  s2;
  uint16_t px;
  uint16_t out_idx = 0U;

  // 1) 컨트롤러 픽셀 단위로 진행하며 mono dot 3개 → RGB565 packing.
  for (i = 0U; i < ctrl_x_count; i++)
  {
    ctrl_x      = (uint16_t)(ctrl_x_start + i);
    mono_x_base = (uint16_t)(ctrl_x * 3U);

    s0 = mono_fb_get_dot((uint16_t)(mono_x_base + 0U), mono_y);
    s1 = mono_fb_get_dot((uint16_t)(mono_x_base + 1U), mono_y);
    s2 = mono_fb_get_dot((uint16_t)(mono_x_base + 2U), mono_y);

    px = mono3_to_rgb565(s0, s1, s2);

    // 2) 빅엔디안(상위 byte 먼저) 송신 포맷.
    out_buf[out_idx++] = (uint8_t)(px >> 8);
    out_buf[out_idx++] = (uint8_t)(px & 0xFFU);
  }
}

/**
 * @brief  컨트롤러 픽셀 사각 영역을 framebuffer 내용으로 송신.
 * @param  ctrl_x0/y0  좌상 컨트롤러 픽셀 (상대, 즉 [0, LOGICAL_WIDTH/HEIGHT) 범위).
 * @param  ctrl_x1/y1  우하 포함 좌표.
 *
 * ram 좌표 = RAM_OFFSET + VIEW_MIN + 상대 컨트롤러 픽셀. (4단계 변환의 마지막 단계)
 */
static void mono_flush_ctrl_rect(uint16_t ctrl_x0, uint16_t ctrl_y0,
                                 uint16_t ctrl_x1, uint16_t ctrl_y1)
{
  uint8_t  tx_line[ST7735S_MONO_LINE_TX_BYTES];   /* 172 byte */
  uint16_t ctrl_x_count;
  uint16_t ram_x_start;
  uint16_t ram_x_end;
  uint16_t tx_bytes;
  uint16_t y;

  // 1) 컨트롤러 픽셀 → RAM X 범위(4단계 변환의 마지막 단계). Y는 dirty span에서 결정.
  ctrl_x_count = (uint16_t)(ctrl_x1 - ctrl_x0 + 1U);
  ram_x_start  = (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_VIEW_X_MIN + ctrl_x0);
  ram_x_end    = (uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_VIEW_X_MIN + ctrl_x1);
  tx_bytes     = (uint16_t)(ctrl_x_count * 2U);

  // 2) 요청 범위 내에서 연속 dirty span 을 찾아 span 단위로 송신.
  //    RAMWR 이 자동 진행하므로 clean line 을 중간에 상태대로 skip 할 수 없음 → span 분할.
  y = ctrl_y0;
  while (y <= ctrl_y1)
  {
    uint16_t span_y0;
    uint16_t span_y1;
    uint16_t ram_y_start;
    uint16_t ram_y_end;
    uint16_t yy;

    // 2-1) clean line 는 skip.
    if (mono_dirty_is_set(y) == 0U)
    {
      y++;
      continue;
    }

    // 2-2) 연속 dirty span 확장.
    span_y0 = y;
    while ((y <= ctrl_y1) && (mono_dirty_is_set(y) != 0U))
    {
      y++;
    }
    span_y1 = (uint16_t)(y - 1U);

    // 2-3) span 원도우 설정 + RAMWR.
    ram_y_start = (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_VIEW_Y_MIN + span_y0);
    ram_y_end   = (uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_VIEW_Y_MIN + span_y1);

    st7735s_set_window(ram_x_start, ram_y_start, ram_x_end, ram_y_end);
    lcd_write_cmd(0x2CU); /* RAMWR */

    // 2-4) span 내 각 라인 packing → SPI burst. CS는 span 동안 유지.
    HW_LCD_DC_Set(1U);
    lcd_select(1U);
    for (yy = span_y0; yy <= span_y1; yy++)
    {
      mono_pack_line_to_tx(yy, ctrl_x0, ctrl_x_count, tx_line);
      (void)HAL_SPI_Transmit(lcd_get_spi_handle(), tx_line, tx_bytes, HAL_MAX_DELAY);
      mono_dirty_clear_line(yy);
    }
    lcd_select(0U);
  }
}

/* ---------- Public API ---------- */

void ST7735S_Drv_ClearMonoBuffer(uint8_t on)
{
  // 1) framebuffer만 채움. flush 하지 않음.
  mono_fb_clear(on);
}

void ST7735S_Drv_DrawMonoDot(uint16_t mono_x, uint16_t mono_y, uint8_t on)
{
  // 1) 사용 가능 영역(MONO_USABLE_*) 밖은 무시. (mono 진입 시점 클램프 1회)
  if ((mono_x >= ST7735S_MONO_USABLE_WIDTH) || (mono_y >= ST7735S_MONO_USABLE_HEIGHT))
  {
    return;
  }

  // 2) framebuffer 1 dot 갱신. flush 하지 않음.
  mono_fb_set_dot(mono_x, mono_y, on);
}

void ST7735S_Drv_FlushMono(void)
{
  // 1) 전체 컨트롤러 픽셀 사각형(상대 좌표)을 한 번에 송신.
  mono_flush_ctrl_rect(0U, 0U,
                       (uint16_t)(ST7735S_LOGICAL_WIDTH  - 1U),
                       (uint16_t)(ST7735S_LOGICAL_HEIGHT - 1U));
}

void ST7735S_Drv_ClearMono(uint8_t on)
{
  // 1) framebuffer 채움.
  mono_fb_clear(on);
  // 2) 즉시 전체 송신. (테스트/디버그/긴급 전용. 일반 화면 구성에는 ClearMonoBuffer 사용)
  ST7735S_Drv_FlushMono();
}

void ST7735S_Drv_FlushMonoRect(uint16_t mono_x0, uint16_t mono_y0,
                               uint16_t mono_x1, uint16_t mono_y1)
{
  uint16_t mx0;
  uint16_t my0;
  uint16_t mx1;
  uint16_t my1;
  uint16_t ctrl_x0;
  uint16_t ctrl_x1;

  // 1) 좌표 정규화 (start <= end).
  if (mono_x0 <= mono_x1) { mx0 = mono_x0; mx1 = mono_x1; }
  else                    { mx0 = mono_x1; mx1 = mono_x0; }

  if (mono_y0 <= mono_y1) { my0 = mono_y0; my1 = mono_y1; }
  else                    { my0 = mono_y1; my1 = mono_y0; }

  // 2) 사용 가능 영역 클램프.
  if (mx1 >= ST7735S_MONO_USABLE_WIDTH)  { mx1 = (uint16_t)(ST7735S_MONO_USABLE_WIDTH  - 1U); }
  if (my1 >= ST7735S_MONO_USABLE_HEIGHT) { my1 = (uint16_t)(ST7735S_MONO_USABLE_HEIGHT - 1U); }
  if (mx0 >= ST7735S_MONO_USABLE_WIDTH)  { return; }   /* 영역 자체가 화면 밖 */
  if (my0 >= ST7735S_MONO_USABLE_HEIGHT) { return; }

  // 3) mono → 컨트롤러 픽셀 경계 정렬.
  //    좌측: 같은 컨트롤러 픽셀의 모든 서브픽셀이 함께 송신되어야 하므로 내림.
  //    우측: 마지막 mono dot을 포함하는 컨트롤러 픽셀까지 송신되도록 올림.
  ctrl_x0 = (uint16_t)(mx0 / 3U);
  ctrl_x1 = (uint16_t)(mx1 / 3U);
  if (ctrl_x1 >= ST7735S_LOGICAL_WIDTH)
  {
    ctrl_x1 = (uint16_t)(ST7735S_LOGICAL_WIDTH - 1U);
  }

  // 4) 컨트롤러 픽셀 사각형으로 송신 위임. (세로는 1:1)
  mono_flush_ctrl_rect(ctrl_x0, my0, ctrl_x1, my1);
}

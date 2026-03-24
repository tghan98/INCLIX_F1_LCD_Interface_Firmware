/*
 * Card_Insert_Test_App.c
 * 
 * 카드 ?�입 감�? �?I2C ?�캔 ?�스???�플리�??�션
 * - PB2(Chip_EN) ?�력 감시
 * - High 감�? ??카드 ?�입?�로 ?�단
 * - 20ms ?��???I2C2 주소 ?�캔
 * - ACK ?�답 ?��????�라 PASS/FAIL ?�정
 */

#include "Card_Insert_Test_App.h"
#include "ST7735S_Drv.h"
#include "string.h"

/* ============================================================================
 * ?��? 변??(main.c?�서 ?�언)
 * ============================================================================ */
extern I2C_HandleTypeDef hi2c2;

/* ============================================================================
 * 매크�??�의
 * ============================================================================ */

#define I2C_SCAN_TIMEOUT_MS             10U        /* I2C ?�캔 ?�?�아??*/
#define CARD_DEBOUNCE_DELAY_MS          20U        /* 카드 감�? ???��??�간 */
#define I2C_ADDRESS_START               0x08U      /* I2C ?�캔 ?�작 주소 (0x08 ~ 0x77) */
#define I2C_ADDRESS_END                 0x77U      /* I2C ?�캔 종료 주소 */

/* UI ?�상 ?�정 */
#define TEXT_GRAY                       0x0FU      /* ?�스???�상 (?�색) */
#define BG_GRAY                         0x00U      /* 배경 ?�상 (검?�색) */
#define TEXT_RGB565                     0xFFFFU
#define BG_RGB565                       0x0000U
/* ?�트 ?�기 �?배치 */
#define CHAR_W                          5U         /* ??글???�비 */
#define CHAR_H                          7U         /* ??글???�이 */
#define CHAR_SPACING                    1U         /* 글??간격 */
#define LINE_STEP                       9U         /* �?간격 */
#define MARGIN_X                        (ST7735S_VIEW_X_MIN + 2U)  /* 좌측 마진: ?�이?�포?�경계+2 */
#define MARGIN_Y                        (ST7735S_VIEW_Y_MIN + 2U)  /* ?�단 마진: ?�이?�포?�경계+2 */

/* ============================================================================
 * ?�적 변??
 * ============================================================================ */

static uint8_t s_frame[ST7735S_DRV_FRAME_BYTES];  /* OLED ?�면 버퍼 */

/* ============================================================================
 * ?�수 ?�언부 (?��? ?�용)
 * ============================================================================ */

static void ClearFrame(uint8_t gray);
static void SetPixel(uint32_t x, uint32_t y, uint8_t gray);
static const uint8_t *GetGlyph(char ch);
static void DrawChar(uint32_t x, uint32_t y, char ch, uint8_t gray);
static void DrawString(uint32_t x, uint32_t y, const char *text, uint8_t gray);
static void DrawHex8(uint32_t x, uint32_t y, uint8_t value, uint8_t gray);
static void UpdateDisplay(void);

static uint8_t ScanI2C(uint8_t *found_addr);

/* ============================================================================
 * ?�수 구현부
 * ============================================================================ */

/*
 * Card_Insert_Test_App_Init()
 * 초기???�수
 */
void Card_Insert_Test_App_Init(void)
{
  /* OLED 초기??*/
  ST7735S_Drv_Init();
  
  /* ?�면 ?�리??*/
  ClearFrame(BG_GRAY);
  
  /* 초기 메시지 ?�시 */
  DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "CARD TEST READY", TEXT_GRAY);
  DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "WAITING", TEXT_GRAY);
  DrawString(MARGIN_X, MARGIN_Y + 3 * LINE_STEP, "PB2 HIGH", TEXT_GRAY);
  DrawString(MARGIN_X, MARGIN_Y + 5 * LINE_STEP, "INSERT CARD", TEXT_GRAY);
  
  UpdateDisplay();
}

/*
 * Card_Insert_Test_App_Run()
 * 메인 루프 - PB2 감시 �?I2C ?�캔
 */
void Card_Insert_Test_App_Run(void)
{
  GPIO_PinState pin_state;
  uint8_t i2c_found_count = 0;
  uint8_t found_addr = 0;
  
  while (1)
  {
    /* 1. PB2(Chip_EN_Pin) ?�력 감시 */
    pin_state = HAL_GPIO_ReadPin(Chip_EN_GPIO_Port, Chip_EN_Pin);
    
    if (pin_state == GPIO_PIN_SET)  /* High 감�? */
    {
      /* 2. 카드 ?�입?�로 ?�단 */
      ClearFrame(BG_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "1 PB2 HIGH", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 1 * LINE_STEP, "CARD DETECTED", TEXT_GRAY);
      UpdateDisplay();
      HAL_Delay(1500);  /* 1.5�??�시 */
      
      /* 3. 20ms ?��?*/
      ClearFrame(BG_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "2 WAIT 20MS", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 1 * LINE_STEP, "DEBOUNCE", TEXT_GRAY);
      UpdateDisplay();
      HAL_Delay(CARD_DEBOUNCE_DELAY_MS);
      HAL_Delay(1000);  /* 추�? 1�??�시 */
      
      /* 4. I2C2 주소 ?�캔 */
      ClearFrame(BG_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "3 SCAN I2C2", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 1 * LINE_STEP, "PB10 PB11", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "0x08 TO 0x77", TEXT_GRAY);
      UpdateDisplay();
      HAL_Delay(1000);  /* 1�??�시 */
      
      i2c_found_count = ScanI2C(&found_addr);
      
      /* 5. 결과 분석 �??�시 */
      ClearFrame(BG_GRAY);
      
      if (i2c_found_count > 0)
      {
        /* PASS - ACK ?�답 ?�음 */
        DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "*** PASS ***", TEXT_GRAY);
        DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "I2C ACK OK", TEXT_GRAY);
        DrawString(MARGIN_X, MARGIN_Y + 3 * LINE_STEP, "ADDR:", TEXT_GRAY);
        DrawHex8(MARGIN_X + 6 * (CHAR_W + CHAR_SPACING), MARGIN_Y + 3 * LINE_STEP, found_addr, TEXT_GRAY);
      }
      else
      {
        /* FAIL - ACK ?�답 ?�음 */
        DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "*** FAIL ***", TEXT_GRAY);
        DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "NO I2C ACK", TEXT_GRAY);
        DrawString(MARGIN_X, MARGIN_Y + 3 * LINE_STEP, "NO DEVICE", TEXT_GRAY);
      }
      
      DrawString(MARGIN_X, MARGIN_Y + 5 * LINE_STEP, "REMOVE CARD", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 6 * LINE_STEP, "TO TEST AGAIN", TEXT_GRAY);
      UpdateDisplay();
      HAL_Delay(3000);  /* 결과�?3초간 ?�시 */
      
      /* 카드 ?�거 ?��?: I2C ?�답 ?��??�까지 ?�링 (PB2?? ?모�??�이?�므로 I2C?�로 ?�재 ?�인) */
      {
        uint8_t dummy_addr = 0U;
        while (ScanI2C(&dummy_addr) > 0U)
        {
          HAL_Delay(200U);
        }
      }
      
      /* ?�시 ?��??�면?�로 복�? */
      ClearFrame(BG_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "CARD TEST READY", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "WAITING", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 3 * LINE_STEP, "PB2 HIGH", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 5 * LINE_STEP, "INSERT CARD", TEXT_GRAY);
      UpdateDisplay();
    }
    
    HAL_Delay(50);  /* 50ms 주기�??�링 */
  }
}

/*
 * ScanI2C()
 * I2C2 버스�??�캔?�여 ?�답?�는 ?�바?�스 검??
 * 
 * [?�자]
 *   found_addr : 찾�? �?번째 주소�??�?�할 ?�인??
 * 
 * [반환�?
 *   찾�? ?�바?�스 개수
 */
static uint8_t ScanI2C(uint8_t *found_addr)
{
  HAL_StatusTypeDef result;
  uint8_t addr;
  uint8_t count = 0;
  
  for (addr = I2C_ADDRESS_START; addr <= I2C_ADDRESS_END; addr++)
  {
    /* I2C ?�바?�스 존재 ?��? ?�인 (7비트 주소�?1비트 ?�쪽 ?�프?? */
    result = HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(addr << 1), 1, I2C_SCAN_TIMEOUT_MS);
    
    if (result == HAL_OK)
    {
      /* ACK ?�답 받음 - ?�바?�스 발견 */
      if (count == 0)
      {
        *found_addr = addr;  /* �?번째 발견??주소 ?�??*/
      }
      count++;
    }
  }
  
  return count;
}

/* ============================================================================
 * ?�면 그리�??�수??
 * ============================================================================ */

/*
 * ClearFrame()
 * ?�레?�버?��? 지?�된 ?�상?�로 초기??
 */
static void ClearFrame(uint8_t gray)
{
  (void)gray;
  ST7735S_Drv_Clear(BG_RGB565);
}

/*
 * SetPixel()
 * ?�레?�버?�의 ?�정 좌표???��?�??�정
 */
static void SetPixel(uint32_t x, uint32_t y, uint8_t gray)
{
  uint32_t index;
  
  if ((x >= ST7735S_DRV_WIDTH) || (y >= ST7735S_DRV_HEIGHT))
  {
    return;
  }
  
  index = (y * (ST7735S_DRV_WIDTH / 2U)) + (x / 2U);
  
  if ((x & 1U) == 0U)
  {
    s_frame[index] = (uint8_t)((s_frame[index] & 0x0F) | ((gray & 0x0FU) << 4));
  }
  else
  {
    s_frame[index] = (uint8_t)((s_frame[index] & 0xF0) | (gray & 0x0FU));
  }
}

/*
 * GetGlyph()
 * 문자 코드???�당?�는 ?�트 글리프 ?�이??반환
 */
static const uint8_t *GetGlyph(char ch)
{
  static const uint8_t glyph_space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t glyph_asterisk[5] = {0x14, 0x08, 0x3E, 0x08, 0x14};
    static const uint8_t glyph_colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
  static const uint8_t glyph_dot[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
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
    static const uint8_t glyph_Y[5] = {0x07, 0x08, 0x70, 0x08, 0x07};
  static const uint8_t glyph_a[5] = {0x20, 0x54, 0x54, 0x54, 0x78};
  static const uint8_t glyph_c[5] = {0x38, 0x44, 0x44, 0x44, 0x20};
  static const uint8_t glyph_d[5] = {0x38, 0x44, 0x44, 0x48, 0x7F};
  static const uint8_t glyph_e[5] = {0x38, 0x54, 0x54, 0x54, 0x18};
  static const uint8_t glyph_g[5] = {0x0C, 0x52, 0x52, 0x52, 0x3E};
  static const uint8_t glyph_h[5] = {0x7F, 0x08, 0x04, 0x04, 0x78};
  static const uint8_t glyph_i[5] = {0x00, 0x44, 0x7D, 0x40, 0x00};
  static const uint8_t glyph_n[5] = {0x7C, 0x08, 0x04, 0x04, 0x78};
  static const uint8_t glyph_o[5] = {0x38, 0x44, 0x44, 0x44, 0x38};
  static const uint8_t glyph_r[5] = {0x7C, 0x08, 0x04, 0x04, 0x08};
  static const uint8_t glyph_s[5] = {0x48, 0x54, 0x54, 0x54, 0x20};
  static const uint8_t glyph_t[5] = {0x04, 0x3F, 0x44, 0x40, 0x20};
  static const uint8_t glyph_v[5] = {0x3C, 0x40, 0x40, 0x20, 0x7C};
  static const uint8_t glyph_w[5] = {0x3C, 0x40, 0x30, 0x40, 0x3C};
    static const uint8_t glyph_x[5] = {0x44, 0x28, 0x10, 0x28, 0x44};
  
  switch (ch)
  {
    case ' ': return glyph_space;
      case '*': return glyph_asterisk;
      case ':': return glyph_colon;
    case '.': return glyph_dot;
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
      case 'Y': return glyph_Y;
    case 'a': return glyph_a;
    case 'c': return glyph_c;
    case 'd': return glyph_d;
    case 'e': return glyph_e;
    case 'g': return glyph_g;
    case 'h': return glyph_h;
    case 'i': return glyph_i;
    case 'n': return glyph_n;
    case 'o': return glyph_o;
    case 'r': return glyph_r;
    case 's': return glyph_s;
    case 't': return glyph_t;
    case 'v': return glyph_v;
    case 'w': return glyph_w;
      case 'x': return glyph_x;
    default: return glyph_space;
  }
}

/*
 * DrawChar()
 * 지?�된 ?�치??문자 그리�?
 */
static void DrawChar(uint32_t x, uint32_t y, char ch, uint8_t gray)
{
  (void)gray;
  ST7735S_Drv_DrawChar5x7((uint16_t)x, (uint16_t)y, ch, TEXT_RGB565, BG_RGB565);
}

/*
 * DrawString()
 * 지?�된 ?�치??문자??그리�?
 */
static void DrawString(uint32_t x, uint32_t y, const char *text, uint8_t gray)
{
  (void)gray;
  ST7735S_Drv_DrawString5x7((uint16_t)x, (uint16_t)y, text, TEXT_RGB565, BG_RGB565);
}

/*
 * DrawHex8()
 * 8비트 값을 16진수�??�시
 */
static void DrawHex8(uint32_t x, uint32_t y, uint8_t value, uint8_t gray)
{
  static const char hex_chars[] = "0123456789ABCDEF";
  
  DrawChar(x, y, '0', gray);
  DrawChar(x + (CHAR_W + CHAR_SPACING), y, 'x', gray);
  DrawChar(x + 2 * (CHAR_W + CHAR_SPACING), y, hex_chars[(value >> 4) & 0x0F], gray);
  DrawChar(x + 3 * (CHAR_W + CHAR_SPACING), y, hex_chars[value & 0x0F], gray);
}

/*
 * UpdateDisplay()
 * ?�레?�버?��? OLED???�송
 */
static void UpdateDisplay(void)
{
  /* Text is drawn directly to ST7735S in DrawString/DrawChar. */
}

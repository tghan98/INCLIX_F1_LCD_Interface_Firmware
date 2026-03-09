/*
 * Card_Insert_Test_App.c
 * 
 * 카드 삽입 감지 및 I2C 스캔 테스트 애플리케이션
 * - PB2(Chip_EN) 입력 감시
 * - High 감지 시 카드 삽입으로 판단
 * - 20ms 대기 후 I2C2 주소 스캔
 * - ACK 응답 여부에 따라 PASS/FAIL 판정
 */

#include "Card_Insert_Test_App.h"
#include "OLED_SSD1322_Drv.h"
#include "string.h"

/* ============================================================================
 * 외부 변수 (main.c에서 선언)
 * ============================================================================ */
extern I2C_HandleTypeDef hi2c2;

/* ============================================================================
 * 매크로 정의
 * ============================================================================ */

#define I2C_SCAN_TIMEOUT_MS             10U        /* I2C 스캔 타임아웃 */
#define CARD_DEBOUNCE_DELAY_MS          20U        /* 카드 감지 후 대기 시간 */
#define I2C_ADDRESS_START               0x08U      /* I2C 스캔 시작 주소 (0x08 ~ 0x77) */
#define I2C_ADDRESS_END                 0x77U      /* I2C 스캔 종료 주소 */

/* UI 색상 설정 */
#define TEXT_GRAY                       0x0FU      /* 텍스트 색상 (흰색) */
#define BG_GRAY                         0x00U      /* 배경 색상 (검정색) */

/* 폰트 크기 및 배치 */
#define CHAR_W                          5U         /* 한 글자 너비 */
#define CHAR_H                          7U         /* 한 글자 높이 */
#define CHAR_SPACING                    1U         /* 글자 간격 */
#define LINE_STEP                       8U         /* 줄 간격 */
#define MARGIN_X                        4U         /* 좌측 마진 */
#define MARGIN_Y                        2U         /* 상단 마진 */

/* ============================================================================
 * 정적 변수
 * ============================================================================ */

static uint8_t s_frame[OLED_SSD1322_FRAME_BYTES];  /* OLED 화면 버퍼 */

/* ============================================================================
 * 함수 선언부 (내부 전용)
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
 * 함수 구현부
 * ============================================================================ */

/*
 * Card_Insert_Test_App_Init()
 * 초기화 함수
 */
void Card_Insert_Test_App_Init(void)
{
  /* OLED 초기화 */
  OLED_SSD1322_Drv_Init();
  
  /* 화면 클리어 */
  ClearFrame(BG_GRAY);
  
  /* 초기 메시지 표시 */
  DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "CARD TEST READY", TEXT_GRAY);
  DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "WAITING", TEXT_GRAY);
  DrawString(MARGIN_X, MARGIN_Y + 3 * LINE_STEP, "PB2 HIGH", TEXT_GRAY);
  DrawString(MARGIN_X, MARGIN_Y + 5 * LINE_STEP, "INSERT CARD", TEXT_GRAY);
  
  UpdateDisplay();
}

/*
 * Card_Insert_Test_App_Run()
 * 메인 루프 - PB2 감시 및 I2C 스캔
 */
void Card_Insert_Test_App_Run(void)
{
  GPIO_PinState pin_state;
  uint8_t i2c_found_count = 0;
  uint8_t found_addr = 0;
  
  while (1)
  {
    /* 1. PB2(Chip_EN_Pin) 입력 감시 */
    pin_state = HAL_GPIO_ReadPin(Chip_EN_GPIO_Port, Chip_EN_Pin);
    
    if (pin_state == GPIO_PIN_SET)  /* High 감지 */
    {
      /* 2. 카드 삽입으로 판단 */
      ClearFrame(BG_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "1 PB2 HIGH", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 1 * LINE_STEP, "CARD DETECTED", TEXT_GRAY);
      UpdateDisplay();
      HAL_Delay(1500);  /* 1.5초 표시 */
      
      /* 3. 20ms 대기 */
      ClearFrame(BG_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "2 WAIT 20MS", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 1 * LINE_STEP, "DEBOUNCE", TEXT_GRAY);
      UpdateDisplay();
      HAL_Delay(CARD_DEBOUNCE_DELAY_MS);
      HAL_Delay(1000);  /* 추가 1초 표시 */
      
      /* 4. I2C2 주소 스캔 */
      ClearFrame(BG_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "3 SCAN I2C2", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 1 * LINE_STEP, "PB10 PB11", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "0x08 TO 0x77", TEXT_GRAY);
      UpdateDisplay();
      HAL_Delay(1000);  /* 1초 표시 */
      
      i2c_found_count = ScanI2C(&found_addr);
      
      /* 5. 결과 분석 및 표시 */
      ClearFrame(BG_GRAY);
      
      if (i2c_found_count > 0)
      {
        /* PASS - ACK 응답 있음 */
        DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "*** PASS ***", TEXT_GRAY);
        DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "I2C ACK OK", TEXT_GRAY);
        DrawString(MARGIN_X, MARGIN_Y + 3 * LINE_STEP, "ADDR:", TEXT_GRAY);
        DrawHex8(MARGIN_X + 6 * (CHAR_W + CHAR_SPACING), MARGIN_Y + 3 * LINE_STEP, found_addr, TEXT_GRAY);
      }
      else
      {
        /* FAIL - ACK 응답 없음 */
        DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "*** FAIL ***", TEXT_GRAY);
        DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "NO I2C ACK", TEXT_GRAY);
        DrawString(MARGIN_X, MARGIN_Y + 3 * LINE_STEP, "NO DEVICE", TEXT_GRAY);
      }
      
      DrawString(MARGIN_X, MARGIN_Y + 5 * LINE_STEP, "REMOVE CARD", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 6 * LINE_STEP, "TO TEST AGAIN", TEXT_GRAY);
      UpdateDisplay();
      HAL_Delay(3000);  /* 결과를 3초간 표시 */
      
      /* 카드 제거 대기 (PB2 Low 될 때까지) */
      while (HAL_GPIO_ReadPin(Chip_EN_GPIO_Port, Chip_EN_Pin) == GPIO_PIN_SET)
      {
        HAL_Delay(100);
      }
      
      /* 다시 대기 화면으로 복귀 */
      ClearFrame(BG_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 0 * LINE_STEP, "CARD TEST READY", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 2 * LINE_STEP, "WAITING", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 3 * LINE_STEP, "PB2 HIGH", TEXT_GRAY);
      DrawString(MARGIN_X, MARGIN_Y + 5 * LINE_STEP, "INSERT CARD", TEXT_GRAY);
      UpdateDisplay();
    }
    
    HAL_Delay(50);  /* 50ms 주기로 폴링 */
  }
}

/*
 * ScanI2C()
 * I2C2 버스를 스캔하여 응답하는 디바이스 검색
 * 
 * [인자]
 *   found_addr : 찾은 첫 번째 주소를 저장할 포인터
 * 
 * [반환값]
 *   찾은 디바이스 개수
 */
static uint8_t ScanI2C(uint8_t *found_addr)
{
  HAL_StatusTypeDef result;
  uint8_t addr;
  uint8_t count = 0;
  
  for (addr = I2C_ADDRESS_START; addr <= I2C_ADDRESS_END; addr++)
  {
    /* I2C 디바이스 존재 여부 확인 (7비트 주소를 1비트 왼쪽 시프트) */
    result = HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(addr << 1), 1, I2C_SCAN_TIMEOUT_MS);
    
    if (result == HAL_OK)
    {
      /* ACK 응답 받음 - 디바이스 발견 */
      if (count == 0)
      {
        *found_addr = addr;  /* 첫 번째 발견된 주소 저장 */
      }
      count++;
    }
  }
  
  return count;
}

/* ============================================================================
 * 화면 그리기 함수들
 * ============================================================================ */

/*
 * ClearFrame()
 * 프레임버퍼를 지정된 색상으로 초기화
 */
static void ClearFrame(uint8_t gray)
{
  uint8_t packed_gray = (uint8_t)(((gray & 0x0FU) << 4) | (gray & 0x0FU));
  memset(s_frame, packed_gray, sizeof(s_frame));
}

/*
 * SetPixel()
 * 프레임버퍼의 특정 좌표에 픽셀값 설정
 */
static void SetPixel(uint32_t x, uint32_t y, uint8_t gray)
{
  uint32_t index;
  
  if ((x >= OLED_SSD1322_WIDTH) || (y >= OLED_SSD1322_HEIGHT))
  {
    return;
  }
  
  index = (y * (OLED_SSD1322_WIDTH / 2U)) + (x / 2U);
  
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
 * 문자 코드에 해당하는 폰트 글리프 데이터 반환
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
 * 지정된 위치에 문자 그리기
 */
static void DrawChar(uint32_t x, uint32_t y, char ch, uint8_t gray)
{
  const uint8_t *glyph = GetGlyph(ch);
  uint32_t col, row;
  
  for (col = 0; col < CHAR_W; col++)
  {
    for (row = 0; row < CHAR_H; row++)
    {
      if ((glyph[col] & (1U << row)) != 0U)
      {
        SetPixel(x + col, y + row, gray);
      }
    }
  }
}

/*
 * DrawString()
 * 지정된 위치에 문자열 그리기
 */
static void DrawString(uint32_t x, uint32_t y, const char *text, uint8_t gray)
{
  uint32_t cursor_x = x;
  
  while (*text != '\0')
  {
    DrawChar(cursor_x, y, *text, gray);
    cursor_x += (CHAR_W + CHAR_SPACING);
    text++;
  }
}

/*
 * DrawHex8()
 * 8비트 값을 16진수로 표시
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
 * 프레임버퍼를 OLED에 전송
 */
static void UpdateDisplay(void)
{
  OLED_SSD1322_Drv_WriteFrame(s_frame);
}

/*
 * EEPROM_Test_App.c
 * 
 * EEPROM ?�스???�플리�??�션
 * - EEPROM ?�체 메모리에 ?�스???�턴(0xAA)???�성
 * - ?�성???�이?��? ?�어??검�?
 * - 진행 ?�황�?결과�?OLED ?�면???�시
 */

#include "EEPROM_Test_App.h"

#include "EEPROM_CAT24C256_Drv.h"
#include "ST7735S_Drv.h"
#include "string.h"

/* ============================================================================
 * 매크�??�의 (?�스???�라미터 �?UI ?�정)
 * ============================================================================ */

#define EEPROM_TEST_PATTERN_VALUE       0xAAU      /* ?�스???�턴�?(0xAA = 10101010 교�? ?�턴) */
#define EEPROM_TEST_PROGRESS_STEP       0x0400U    /* 진행?�황 ?�데?�트 주기 (1KB마다) */

/* UI ?�상 ?�정 (OLED 그레?�스케??4비트) */
#define EEPROM_TEST_TEXT_GRAY           0x0FU      /* ?�스???�상 (?�색) */
#define EEPROM_TEST_BG_GRAY             0x00U      /* 배경 ?�상 (검?�색) */
#define EEPROM_TEST_TEXT_RGB565         0xFFFFU
#define EEPROM_TEST_BG_RGB565           0x0000U
/* ?�트 ?�기 �?배치 */
#define EEPROM_TEST_CHAR_W              5U         /* ??글???�비 (?��?) */
#define EEPROM_TEST_CHAR_H              7U         /* ??글???�이 (?��?) */
#define EEPROM_TEST_CHAR_SPACING        1U         /* 글??간격 (?��?) */
#define EEPROM_TEST_LINE_STEP           8U         /* �?간격 (?��?) */
#define EEPROM_TEST_MARGIN_X            (ST7735S_VIEW_X_MIN + 2U)         /* 좌측 마진 (픽셀) */
#define EEPROM_TEST_MARGIN_Y            (ST7735S_VIEW_Y_MIN + 2U)         /* 상단 마진 (픽셀) */

/* ============================================================================
 * ?�???�의
 * ============================================================================ */

/* EEPROM ?�스?�의 3?�계 ?�이�?*/
typedef enum
{
  EEPROM_TEST_PHASE_WRITE = 0,    /* Phase 1: EEPROM???�이???�성 */
  EEPROM_TEST_PHASE_READ,         /* Phase 2: EEPROM?�서 ?�이???�기 */
  EEPROM_TEST_PHASE_VERIFY        /* Phase 3: ?��? ?�이??검�?*/
} EEPROM_TestPhase_t;

/* ============================================================================
 * ?�적 변??(?�태 버퍼)
 * ============================================================================ */

static uint8_t s_frame[ST7735S_DRV_FRAME_BYTES];  /* OLED ?�면 버퍼 (?�레?�버?? */
static uint8_t s_page_buffer[EEPROM_CAT24C256_PAGE_SIZE];  /* EEPROM ?�기/?�기 버퍼 */


/* ============================================================================
 * ?�수 ?�언부 (?�역: public ?�수??
 * ============================================================================ */

void EEPROM_Test_App_Init(void);
void EEPROM_Test_App_RunOnce(void);

/* ============================================================================
 * ?�수 ?�언부 (?��? ?�용: private ?�수??
 * ============================================================================ */

/* ?�레??버퍼 관??*/
static void EEPROM_Test_ClearFrame(uint8_t gray);
static void EEPROM_Test_SetPixel(uint32_t x, uint32_t y, uint8_t gray);

static const uint8_t *EEPROM_Test_GetGlyph(char ch);
static void EEPROM_Test_DrawChar(uint32_t x, uint32_t y, char ch, uint8_t gray);
static void EEPROM_Test_DrawString(uint32_t x, uint32_t y, const char *text, uint8_t gray);
static void EEPROM_Test_DrawHex8(uint32_t x, uint32_t y, uint8_t value, uint8_t gray);
static void EEPROM_Test_DrawHex16(uint32_t x, uint32_t y, uint16_t value, uint8_t gray);
static void EEPROM_Test_DrawHex32(uint32_t x, uint32_t y, uint32_t value, uint8_t gray);

/* ?�면 ?�시 관??*/
static void EEPROM_Test_ShowStartScreen(void);
static void EEPROM_Test_ShowProgressScreen(const char *phase, uint16_t addr);
static void EEPROM_Test_ShowPassScreen(void);
static void EEPROM_Test_ShowFailScreen(EEPROM_TestPhase_t phase,
                                       uint16_t addr,
                                       uint8_t expected,
                                       uint8_t actual,
                                       uint32_t error_code);
static void EEPROM_Test_ShowDriverFailScreen(EEPROM_TestPhase_t phase, uint16_t addr);
static const char *EEPROM_Test_GetPhaseText(EEPROM_TestPhase_t phase);

/* ============================================================================
 * ?�수 구현부
 * ============================================================================ */

/*
 * EEPROM_Test_ClearFrame()
 * ?�레?�버?��? 지?�된 ?�상?�로 초기??
 * 
 * OLED_SSD1322??4비트 그레?�스케?�을 ?�용?��?�? ?�나??바이?�에 2�??��????�?�됨
 * ?? 0xAB = ?�위 4비트 'A' (좌측 ?��?) + ?�위 4비트 'B' (?�측 ?��?)
 * 
 * [?�자]
 *   gray : 4비트 그레?�스케??�?(0x0 = 검?? 0xF = ?�색)
 */
static void EEPROM_Test_ClearFrame(uint8_t gray)
{
  (void)gray;
  ST7735S_Drv_Clear(EEPROM_TEST_BG_RGB565);
}

/*
 * EEPROM_Test_SetPixel()
 * ?�레?�버?�의 ?�정 좌표???��?�??�정
 * 
 * OLED_SSD1322 ?�면 구조:
 * - 가�?256 ?��? × ?�로 64 ?��?
 * - 4비트 그레?�스케??(?��????�블)
 * - �?바이?�에 좌우 2�??��? ?�??
 * 
 * [?�자]
 *   x, y : ?��? 좌표 (0 ~ 255, 0 ~ 63)
 *   gray : 4비트 그레?�스케??�?(0x0 ~ 0xF)
 */
static void EEPROM_Test_SetPixel(uint32_t x, uint32_t y, uint8_t gray)
{
  uint32_t index;

  /* 경계�?체크 */
  if ((x >= ST7735S_DRV_WIDTH) || (y >= ST7735S_DRV_HEIGHT))
  {
    return;
  }

  /* ?�레?�버???�덱??계산
   * ???��? 256/2 = 128 바이??(2?��? = 1바이??
   * ?�라?? index = (???�작?�치) + (?�위�?/ 2)
   */
  index = (y * (ST7735S_DRV_WIDTH / 2U)) + (x / 2U);
  
  /* 짝수 좌표(좌측 ?��?) vs ?�??좌표(?�측 ?��?) 처리 */
  if ((x & 1U) == 0U)
  {
    /* 좌측 ?��? (?�위 4비트): ?�위 4비트�?보존?�고 ?�위???�정 */
    s_frame[index] = (uint8_t)((s_frame[index] & 0x0F) | ((gray & 0x0FU) << 4));
  }
  else
  {
    /* ?�측 ?��? (?�위 4비트): ?�위 4비트�?보존?�고 ?�위???�정 */
    s_frame[index] = (uint8_t)((s_frame[index] & 0xF0) | (gray & 0x0FU));
  }
}


/*
 * EEPROM_Test_GetGlyph()
 * 문자 코드???�당?�는 ?�트 글리프 ?�이??반환
 * 
 * 글리프 ?�식: 5바이??배열
 * - �?바이?�는 글?�의 ????column)???��???
 * - �?비트(0~6)????row)???��????��???
 * - 1 = ?��? ?�시, 0 = ?��? 미표??
 * 
 * ?��? ?�어 glyph_0 = {0x3E, 0x51, 0x49, 0x45, 0x3E}???�자 '0' 모양
 * 
 * [?�자]
 *   ch : ASCII 문자코드
 * 
 * [반환�?
 *   ?�당 문자??5바이??글리프 ?�인?? ?�으�?공백 글리프
 */
static const uint8_t *EEPROM_Test_GetGlyph(char ch)
{
  /* 글리프 ?�트 ?�이??(5?��? ?�비 × 7?��? ?�이) */
  static const uint8_t glyph_space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
  static const uint8_t glyph_slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
  static const uint8_t glyph_colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
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
  static const uint8_t glyph_D[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
  static const uint8_t glyph_E[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
  static const uint8_t glyph_F[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
  static const uint8_t glyph_G[5] = {0x3E, 0x41, 0x49, 0x49, 0x7A};
  static const uint8_t glyph_I[5] = {0x00, 0x41, 0x7F, 0x41, 0x00};
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

  /* 문자???�당?�는 글리프 반환 */
  switch (ch)
  {
    case ' ': return glyph_space;
    case '/': return glyph_slash;
    case ':': return glyph_colon;
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
    case 'D': return glyph_D;
    case 'E': return glyph_E;
    case 'F': return glyph_F;
    case 'G': return glyph_G;
    case 'I': return glyph_I;
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
    default:  return glyph_space;  /* ?�의?��? ?��? 문자??공백 반환 */
  }
}


/*
 * EEPROM_Test_DrawChar()
 * 지?�한 좌표???�일 문자�??�레?�버?�에 그리�?
 * 
 * ?�작 ?�리:
 * 1. 문자??글리프 ?�이??조회 (5바이??
 * 2. �???col)마다:
 *    - ?�당 ?�의 바이?��? ?�인
 *    - �???row)??비트�??�인
 *    - 비트가 1?�면 ?�당 ?��???SetPixel() ?�출�?그리�?
 * 
 * [?�자]
 *   x, y : 문자??좌상??좌표
 *   ch   : 출력??문자
 *   gray : 그레?�스케??�?
 */
static void EEPROM_Test_DrawChar(uint32_t x, uint32_t y, char ch, uint8_t gray)
{
  (void)gray;
  ST7735S_Drv_DrawChar5x7((uint16_t)x, (uint16_t)y, ch, EEPROM_TEST_TEXT_RGB565, EEPROM_TEST_BG_RGB565);
}

/*
 * EEPROM_Test_DrawString()
 * 지?�한 좌표부??문자?�을 ?�레?�버?�에 그리�?
 * 
 * ?�작: �?문자�??�회?�며 DrawChar() ?�출, x좌표�?갱신?�며 ?�음 문자 배치
 * 
 * [?�자]
 *   x, y  : 문자???�작 좌표
 *   text  : ??종료 문자??
 *   gray  : 그레?�스케??�?
 */
static void EEPROM_Test_DrawString(uint32_t x, uint32_t y, const char *text, uint8_t gray)
{
  (void)gray;
  ST7735S_Drv_DrawString5x7((uint16_t)x, (uint16_t)y, text, EEPROM_TEST_TEXT_RGB565, EEPROM_TEST_BG_RGB565);
}

/*
 * EEPROM_Test_DrawHex8()
 * 8비트 값을 16진수 2글?�로 출력 (?? 0xAA ??"AA")
 * 
 * [?�자]
 *   x, y  : ?�작 좌표
 *   value : 출력??8비트 ?�수�?
 *   gray  : 그레?�스케??
 */
static void EEPROM_Test_DrawHex8(uint32_t x, uint32_t y, uint8_t value, uint8_t gray)
{
  static const char hex_digits[] = "0123456789ABCDEF";
  char text[3];

  text[0] = hex_digits[(value >> 4) & 0x0F];  /* ?�위 4비트 */
  text[1] = hex_digits[value & 0x0F];         /* ?�위 4비트 */
  text[2] = '\0';
  EEPROM_Test_DrawString(x, y, text, gray);
}

/*
 * EEPROM_Test_DrawHex16()
 * 16비트 값을 16진수 4글?�로 출력 (?? 0x1234 ??"1234")
 * 
 * [?�자]
 *   x, y  : ?�작 좌표
 *   value : 출력??16비트 ?�수�?
 *   gray  : 그레?�스케??
 */
static void EEPROM_Test_DrawHex16(uint32_t x, uint32_t y, uint16_t value, uint8_t gray)
{
  static const char hex_digits[] = "0123456789ABCDEF";
  char text[5];

  text[0] = hex_digits[(value >> 12) & 0x0F];  /* 비트 12-15 */
  text[1] = hex_digits[(value >> 8) & 0x0F];   /* 비트 8-11 */
  text[2] = hex_digits[(value >> 4) & 0x0F];   /* 비트 4-7 */
  text[3] = hex_digits[value & 0x0F];          /* 비트 0-3 */
  text[4] = '\0';
  EEPROM_Test_DrawString(x, y, text, gray);
}

/*
 * EEPROM_Test_DrawHex32()
 * 32비트 값을 16진수 8글?�로 출력 (?? 0x12345678 ??"12345678")
 * 
 * [?�자]
 *   x, y  : ?�작 좌표
 *   value : 출력??32비트 ?�수�?
 *   gray  : 그레?�스케??
 */
static void EEPROM_Test_DrawHex32(uint32_t x, uint32_t y, uint32_t value, uint8_t gray)
{
  static const char hex_digits[] = "0123456789ABCDEF";
  char text[9];
  uint32_t idx;

  /* 가???�위 ?�블부???�작????��?�로 문자 배치 */
  for (idx = 0U; idx < 8U; idx++)
  {
    text[7U - idx] = hex_digits[value & 0x0FU];
    value >>= 4;
  }
  text[8] = '\0';
  EEPROM_Test_DrawString(x, y, text, gray);
}


/*
 * EEPROM_Test_ShowStartScreen()
 * ?�스???�작 ?�면 ?�시
 * 
 * ?�시 ?�용:
 * - "EEPROM TEST" ?�목
 * - "PATTERN" �??�스???�턴�?(0xAA)
 * - "START" ?�스??
 */
static void EEPROM_Test_ShowStartScreen(void)
{
  EEPROM_Test_ClearFrame(EEPROM_TEST_BG_GRAY);
  
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 0U), "EEPROM TEST", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 2U), "PATTERN", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawHex8(EEPROM_TEST_MARGIN_X + (8U * (EEPROM_TEST_CHAR_W + EEPROM_TEST_CHAR_SPACING)),
                       EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 2U),
                       EEPROM_TEST_PATTERN_VALUE,
                       EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 4U), "START", EEPROM_TEST_TEXT_GRAY);
  
  /* ?�레?�버?��? OLED???�송 */
  /* Direct draw mode: no frame push required. */
}

/*
 * EEPROM_Test_ShowProgressScreen()
 * WRITE/VERIFY 진행 �?진행?�황 ?�시 ?�면
 * 
 * ?�시 ?�용:
 * - "EEPROM TEST" ?�목
 * - "PATTERN" �??�스???�턴�?(0xAA)
 * - ?�재 ?�이�?("WRITE" ?�는 "VERIFY")
 * - ?�재 주소 (16진수 4글??
 * 
 * [?�자]
 *   phase : "WRITE" ?�는 "VERIFY" 문자??
 *   addr  : ?�재 진행 중인 EEPROM 주소
 */
static void EEPROM_Test_ShowProgressScreen(const char *phase, uint16_t addr)
{
  EEPROM_Test_ClearFrame(EEPROM_TEST_BG_GRAY);
  
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 0U), "EEPROM TEST", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 2U), "PATTERN", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawHex8(EEPROM_TEST_MARGIN_X + (8U * (EEPROM_TEST_CHAR_W + EEPROM_TEST_CHAR_SPACING)),
                       EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 2U),
                       EEPROM_TEST_PATTERN_VALUE,
                       EEPROM_TEST_TEXT_GRAY);
  
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 3U), phase, EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 4U), "ADDR", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawHex16(EEPROM_TEST_MARGIN_X + (5U * (EEPROM_TEST_CHAR_W + EEPROM_TEST_CHAR_SPACING)),
                        EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 4U),
                        addr,
                        EEPROM_TEST_TEXT_GRAY);
  
  /* Direct draw mode: no frame push required. */
}

/*
 * EEPROM_Test_ShowPassScreen()
 * ?�스???�공 결과 ?�면
 * 
 * ?�시 ?�용:
 * - "EEPROM TEST" ?�목
 * - "PATTERN" �??�스???�턴�?
 * - "PASS" 결과
 */
static void EEPROM_Test_ShowPassScreen(void)
{
  EEPROM_Test_ClearFrame(EEPROM_TEST_BG_GRAY);
  
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 0U), "EEPROM TEST", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 2U), "PATTERN", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawHex8(EEPROM_TEST_MARGIN_X + (8U * (EEPROM_TEST_CHAR_W + EEPROM_TEST_CHAR_SPACING)),
                       EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 2U),
                       EEPROM_TEST_PATTERN_VALUE,
                       EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 4U), "PASS", EEPROM_TEST_TEXT_GRAY);
  
  /* Direct draw mode: no frame push required. */
}


/*
 * EEPROM_Test_GetPhaseText()
 * ?�스???�이�?enum 값에 ?�당?�는 ?�스??반환
 * 
 * [?�자]
 *   phase : EEPROM_TestPhase_t enum �?
 * 
 * [반환�?
 *   "WRITE", "READ", "VERIFY", ?�는 "FAIL"
 */
static const char *EEPROM_Test_GetPhaseText(EEPROM_TestPhase_t phase)
{
  switch (phase)
  {
    case EEPROM_TEST_PHASE_WRITE:
      return "WRITE";
    case EEPROM_TEST_PHASE_READ:
      return "READ";
    case EEPROM_TEST_PHASE_VERIFY:
      return "VERIFY";
    default:
      return "FAIL";
  }
}

/*
 * EEPROM_Test_ShowFailScreen()
 * ?�스???�패 결과 ?�세 ?�보 ?�면
 * 
 * ?�시 ?�용:
 * - "EEPROM TEST" ?�목
 * - "FAIL" 결과
 * - ?�패???�이�?(WRITE/READ/VERIFY)
 * - ?�패 주소 (16진수 4글??
 * - EXP (?�상�? 8비트 16진수)
 * - GOT (?�제�? 8비트 16진수)
 * - ERR (?�러 코드, 32비트 16진수)
 * 
 * [?�자]
 *   phase     : ?�패???�이�?
 *   addr      : ?�패??주소
 *   expected  : ?�상?�던 바이???�이??
 *   actual    : ?�제 ?��? 바이???�이??
 *   error_code : ?�라?�버 ?�러 코드
 */
static void EEPROM_Test_ShowFailScreen(EEPROM_TestPhase_t phase,
                                       uint16_t addr,
                                       uint8_t expected,
                                       uint8_t actual,
                                       uint32_t error_code)
{
  EEPROM_Test_ClearFrame(EEPROM_TEST_BG_GRAY);
  
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 0U), "EEPROM TEST", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 1U), "FAIL", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 2U), EEPROM_Test_GetPhaseText(phase), EEPROM_TEST_TEXT_GRAY);
  
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 3U), "ADDR", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawHex16(EEPROM_TEST_MARGIN_X + (5U * (EEPROM_TEST_CHAR_W + EEPROM_TEST_CHAR_SPACING)),
                        EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 3U),
                        addr,
                        EEPROM_TEST_TEXT_GRAY);
  
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 4U), "EXP", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawHex8(EEPROM_TEST_MARGIN_X + (4U * (EEPROM_TEST_CHAR_W + EEPROM_TEST_CHAR_SPACING)),
                       EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 4U),
                       expected,
                       EEPROM_TEST_TEXT_GRAY);
  
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 5U), "GOT", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawHex8(EEPROM_TEST_MARGIN_X + (4U * (EEPROM_TEST_CHAR_W + EEPROM_TEST_CHAR_SPACING)),
                       EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 5U),
                       actual,
                       EEPROM_TEST_TEXT_GRAY);
  
  EEPROM_Test_DrawString(EEPROM_TEST_MARGIN_X, EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 6U), "ERR", EEPROM_TEST_TEXT_GRAY);
  EEPROM_Test_DrawHex32(EEPROM_TEST_MARGIN_X + (4U * (EEPROM_TEST_CHAR_W + EEPROM_TEST_CHAR_SPACING)),
                        EEPROM_TEST_MARGIN_Y + (EEPROM_TEST_LINE_STEP * 6U),
                        error_code,
                        EEPROM_TEST_TEXT_GRAY);
  
  /* Direct draw mode: no frame push required. */
}

/*
 * EEPROM_Test_ShowDriverFailScreen()
 * EEPROM ?�라?�버 ?�러 발생 ??보여주는 ?�면
 * 
 * ?�상�?= EEPROM_TEST_PATTERN_VALUE (0xAA)
 * ?�제�?= 0xEE (?�라?�버 ?�러 ?�시�?
 * ?�러코드 = ?�라?�버??마�?�??�러�?
 * 
 * [?�자]
 *   phase : ?�러 발생???�이�?
 *   addr  : ?�러 발생 주소
 */
static void EEPROM_Test_ShowDriverFailScreen(EEPROM_TestPhase_t phase, uint16_t addr)
{
  EEPROM_Test_ShowFailScreen(phase,
                             addr,
                             EEPROM_TEST_PATTERN_VALUE,
                             0xEEU,                                      /* ?�라?�버 ?�러 ?�시 */
                             EEPROM_CAT24C256_Drv_GetLastError());       /* ?�제 ?�러 코드 조회 */
}


/* ============================================================================
 * 공개 ?�터?�이??(Public API)
 * ============================================================================ */

/*
 * EEPROM_Test_App_Init()
 * 
 * EEPROM ?�스???�플리�??�션 초기??
 * User_Main_Init()?�서 ?�출??
 * 
 * ??��:
 * 1. OLED ?�면 ?�라?�버 초기??
 * 2. ?�작 ?�면 ?�시
 * 
 * ?�출 ?�서 (??��?�운):
 * main() 
 *  ??User_Main_Init() 
 *    ??EEPROM_Test_App_Init()
 *      ??ST7735S_Drv_Init()             [?�드?�어 ?�라?�버]
 *      ??EEPROM_Test_ShowStartScreen()   [UI ?�시]
 */
void EEPROM_Test_App_Init(void)
{
  ST7735S_Drv_Init();             /* ST7735S ?�라?�버 초기??*/
  EEPROM_Test_ShowStartScreen();   /* ?�작 ?�면 ?�시 */
}

/*
 * EEPROM_Test_App_RunOnce()
 * 
 * EEPROM ?�스???�행 (WRITE ??READ & VERIFY ??RESULT)
 * User_Main_Run()?�서 ??번만 ?�출??
 * 
 * ?�차:
 * 1) page buffer�?0xAA�?채�?
 * 2) ?�체 EEPROM???�이지 ?�위�?WRITE
 * 3) ?�시 READ ??0xAA?� 비교(VERIFY)
 * 4) ?�러 ??FAIL ?�면, 모두 ?�치?�면 PASS ?�면
 */
void EEPROM_Test_App_RunOnce(void)
{
  uint16_t addr;
  uint16_t offset;
  HAL_StatusTypeDef hal_status;

  /* 1. ?�스???�이??준�?*/
  memset(s_page_buffer, EEPROM_TEST_PATTERN_VALUE, sizeof(s_page_buffer));
  HAL_Delay(300U);  /* ?�드?�어 ?�정???��?*/

  /* PHASE 1: WRITE - EEPROM ?�체???�스???�턴(0xAA) 기록 */
  for (addr = 0U; addr < EEPROM_CAT24C256_TOTAL_BYTES; addr += EEPROM_CAT24C256_PAGE_SIZE)
  {
    /* 1KB 주기마다 진행?�황 ?�면 ?�데?�트 */
    if ((addr % EEPROM_TEST_PROGRESS_STEP) == 0U)
    {
      EEPROM_Test_ShowProgressScreen("WRITE", addr);
    }

    /* EEPROM Driver ?�출: 64바이???�이지 ?�위�?기록
      * 주소: addr, ?�이?? s_page_buffer (0xAA × 64), ?�기: 64바이??*/
    hal_status = EEPROM_CAT24C256_Drv_WritePage(addr, s_page_buffer, EEPROM_CAT24C256_PAGE_SIZE);
    
    /* ?�러 처리 */
    if (hal_status != HAL_OK)
    {
      EEPROM_Test_ShowDriverFailScreen(EEPROM_TEST_PHASE_WRITE, addr);
      return;  /* ?�스??종료 */
    }
  }

  /* PHASE 2: READ & VERIFY - EEPROM?�서 ?�이???�고 검�?*/
  for (addr = 0U; addr < EEPROM_CAT24C256_TOTAL_BYTES; addr += EEPROM_CAT24C256_PAGE_SIZE)
  {
    /* 1KB 주기마다 진행?�황 ?�면 ?�데?�트 */
    if ((addr % EEPROM_TEST_PROGRESS_STEP) == 0U)
    {
      EEPROM_Test_ShowProgressScreen("VERIFY", addr);
    }

    /* EEPROM Driver ?�출: addr 주소?�서 64바이???�기 */
    hal_status = EEPROM_CAT24C256_Drv_Read(addr, s_page_buffer, EEPROM_CAT24C256_PAGE_SIZE);
    
    /* ?�라?�버 ?�러 처리 */
    if (hal_status != HAL_OK)
    {
      EEPROM_Test_ShowDriverFailScreen(EEPROM_TEST_PHASE_READ, addr);
      return;  /* ?�스??종료 */
    }

    /* ?��? ?�이??검�? �?바이?��? 0xAA?��? ?�인 */
    for (offset = 0U; offset < EEPROM_CAT24C256_PAGE_SIZE; offset++)
    {
      if (s_page_buffer[offset] != EEPROM_TEST_PATTERN_VALUE)
      {
        /* ?�이??불일�????�스???�패 */
        EEPROM_Test_ShowFailScreen(EEPROM_TEST_PHASE_VERIFY,
                                   (uint16_t)(addr + offset),           /* ?�패??주소 */
                                   EEPROM_TEST_PATTERN_VALUE,           /* ?�상�?(0xAA) */
                                   s_page_buffer[offset],               /* ?�제�?*/
                                   EEPROM_CAT24C256_Drv_GetLastError()); /* ?�러 코드 */
        return;  /* ?�스??종료 */
      }
    }
  }

  /* 모든 검�??�료 ???�스???�공 */
  EEPROM_Test_ShowPassScreen();
}

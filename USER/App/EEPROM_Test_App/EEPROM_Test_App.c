/*
 * EEPROM_Test_App.c
 * 
 * EEPROM 테스트 애플리케이션
 * - EEPROM 전체 메모리에 테스트 패턴(0xAA)을 작성
 * - 작성한 데이터를 읽어서 검증
 * - 진행 상황과 결과를 OLED 화면에 표시
 */

#include "EEPROM_Test_App.h"

#include "EEPROM_CAT24C256_Drv.h"
#include "OLED_SSD1322_Drv.h"
#include "string.h"

/* ============================================================================
 * 매크로 정의 (테스트 파라미터 및 UI 설정)
 * ============================================================================ */

#define EEPROM_TEST_PATTERN_VALUE       0xAAU      /* 테스트 패턴값 (0xAA = 10101010 교대 패턴) */
#define EEPROM_TEST_PROGRESS_STEP       0x0400U    /* 진행상황 업데이트 주기 (1KB마다) */

/* UI 색상 설정 (OLED 그레이스케일 4비트) */
#define EEPROM_TEST_TEXT_GRAY           0x0FU      /* 텍스트 색상 (흰색) */
#define EEPROM_TEST_BG_GRAY             0x00U      /* 배경 색상 (검정색) */

/* 폰트 크기 및 배치 */
#define EEPROM_TEST_CHAR_W              5U         /* 한 글자 너비 (픽셀) */
#define EEPROM_TEST_CHAR_H              7U         /* 한 글자 높이 (픽셀) */
#define EEPROM_TEST_CHAR_SPACING        1U         /* 글자 간격 (픽셀) */
#define EEPROM_TEST_LINE_STEP           8U         /* 줄 간격 (픽셀) */
#define EEPROM_TEST_MARGIN_X            4U         /* 좌측 마진 (픽셀) */
#define EEPROM_TEST_MARGIN_Y            2U         /* 상단 마진 (픽셀) */

/* ============================================================================
 * 타입 정의
 * ============================================================================ */

/* EEPROM 테스트의 3단계 페이즈 */
typedef enum
{
  EEPROM_TEST_PHASE_WRITE = 0,    /* Phase 1: EEPROM에 데이터 작성 */
  EEPROM_TEST_PHASE_READ,         /* Phase 2: EEPROM에서 데이터 읽기 */
  EEPROM_TEST_PHASE_VERIFY        /* Phase 3: 읽은 데이터 검증 */
} EEPROM_TestPhase_t;

/* ============================================================================
 * 정적 변수 (상태 버퍼)
 * ============================================================================ */

static uint8_t s_frame[OLED_SSD1322_FRAME_BYTES];  /* OLED 화면 버퍼 (프레임버퍼) */
static uint8_t s_page_buffer[EEPROM_CAT24C256_PAGE_SIZE];  /* EEPROM 읽기/쓰기 버퍼 */


/* ============================================================================
 * 함수 선언부 (전역: public 함수들)
 * ============================================================================ */

void EEPROM_Test_App_Init(void);
void EEPROM_Test_App_RunOnce(void);

/* ============================================================================
 * 함수 선언부 (내부 전용: private 함수들)
 * ============================================================================ */

/* 프레임 버퍼 관련 */
static void EEPROM_Test_ClearFrame(uint8_t gray);
static void EEPROM_Test_SetPixel(uint32_t x, uint32_t y, uint8_t gray);

/* 문자 그리기 관련 */
static const uint8_t *EEPROM_Test_GetGlyph(char ch);
static void EEPROM_Test_DrawChar(uint32_t x, uint32_t y, char ch, uint8_t gray);
static void EEPROM_Test_DrawString(uint32_t x, uint32_t y, const char *text, uint8_t gray);
static void EEPROM_Test_DrawHex8(uint32_t x, uint32_t y, uint8_t value, uint8_t gray);
static void EEPROM_Test_DrawHex16(uint32_t x, uint32_t y, uint16_t value, uint8_t gray);
static void EEPROM_Test_DrawHex32(uint32_t x, uint32_t y, uint32_t value, uint8_t gray);

/* 화면 표시 관련 */
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
 * 함수 구현부
 * ============================================================================ */

/*
 * EEPROM_Test_ClearFrame()
 * 프레임버퍼를 지정된 색상으로 초기화
 * 
 * OLED_SSD1322는 4비트 그레이스케일을 사용하므로, 하나의 바이트에 2개 픽셀이 저장됨
 * 예: 0xAB = 상위 4비트 'A' (좌측 픽셀) + 하위 4비트 'B' (우측 픽셀)
 * 
 * [인자]
 *   gray : 4비트 그레이스케일 값 (0x0 = 검정, 0xF = 흰색)
 */
static void EEPROM_Test_ClearFrame(uint8_t gray)
{
  uint8_t packed_gray;

  /* 4비트 값을 8비트로 확장 (상위 4비트와 하위 4비트에 같은 값 배치) */
  packed_gray = (uint8_t)(((gray & 0x0FU) << 4) | (gray & 0x0FU));
  
  /* 전체 프레임버퍼를 동일한 값으로 채우기 */
  memset(s_frame, packed_gray, sizeof(s_frame));
}

/*
 * EEPROM_Test_SetPixel()
 * 프레임버퍼의 특정 좌표에 픽셀값 설정
 * 
 * OLED_SSD1322 화면 구조:
 * - 가로 256 픽셀 × 세로 64 픽셀
 * - 4비트 그레이스케일 (픽셀당 니블)
 * - 각 바이트에 좌우 2개 픽셀 저장
 * 
 * [인자]
 *   x, y : 픽셀 좌표 (0 ~ 255, 0 ~ 63)
 *   gray : 4비트 그레이스케일 값 (0x0 ~ 0xF)
 */
static void EEPROM_Test_SetPixel(uint32_t x, uint32_t y, uint8_t gray)
{
  uint32_t index;

  /* 경계값 체크 */
  if ((x >= OLED_SSD1322_WIDTH) || (y >= OLED_SSD1322_HEIGHT))
  {
    return;
  }

  /* 프레임버퍼 인덱스 계산
   * 한 행은 256/2 = 128 바이트 (2픽셀 = 1바이트)
   * 따라서: index = (행 시작위치) + (열위치 / 2)
   */
  index = (y * (OLED_SSD1322_WIDTH / 2U)) + (x / 2U);
  
  /* 짝수 좌표(좌측 픽셀) vs 홀수 좌표(우측 픽셀) 처리 */
  if ((x & 1U) == 0U)
  {
    /* 좌측 픽셀 (상위 4비트): 하위 4비트를 보존하고 상위에 설정 */
    s_frame[index] = (uint8_t)((s_frame[index] & 0x0F) | ((gray & 0x0FU) << 4));
  }
  else
  {
    /* 우측 픽셀 (하위 4비트): 상위 4비트를 보존하고 하위에 설정 */
    s_frame[index] = (uint8_t)((s_frame[index] & 0xF0) | (gray & 0x0FU));
  }
}


/*
 * EEPROM_Test_GetGlyph()
 * 문자 코드에 해당하는 폰트 글리프 데이터 반환
 * 
 * 글리프 형식: 5바이트 배열
 * - 각 바이트는 글자의 한 열(column)을 나타냄
 * - 각 비트(0~6)는 행(row)의 픽셀을 나타냄
 * - 1 = 픽셀 표시, 0 = 픽셀 미표시
 * 
 * 예를 들어 glyph_0 = {0x3E, 0x51, 0x49, 0x45, 0x3E}는 숫자 '0' 모양
 * 
 * [인자]
 *   ch : ASCII 문자코드
 * 
 * [반환값]
 *   해당 문자의 5바이트 글리프 포인터, 없으면 공백 글리프
 */
static const uint8_t *EEPROM_Test_GetGlyph(char ch)
{
  /* 글리프 폰트 데이터 (5픽셀 너비 × 7픽셀 높이) */
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

  /* 문자에 해당하는 글리프 반환 */
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
    default:  return glyph_space;  /* 정의되지 않은 문자는 공백 반환 */
  }
}


/*
 * EEPROM_Test_DrawChar()
 * 지정한 좌표에 단일 문자를 프레임버퍼에 그리기
 * 
 * 동작 원리:
 * 1. 문자의 글리프 데이터 조회 (5바이트)
 * 2. 각 열(col)마다:
 *    - 해당 열의 바이트를 확인
 *    - 각 행(row)의 비트를 확인
 *    - 비트가 1이면 해당 픽셀을 SetPixel() 호출로 그리기
 * 
 * [인자]
 *   x, y : 문자의 좌상단 좌표
 *   ch   : 출력할 문자
 *   gray : 그레이스케일 값
 */
static void EEPROM_Test_DrawChar(uint32_t x, uint32_t y, char ch, uint8_t gray)
{
  uint32_t col;
  uint32_t row;
  const uint8_t *glyph;

  glyph = EEPROM_Test_GetGlyph(ch);
  
  /* 글리프의 각 열 처리 */
  for (col = 0U; col < EEPROM_TEST_CHAR_W; col++)
  {
    /* 각 행의 비트 확인 */
    for (row = 0U; row < EEPROM_TEST_CHAR_H; row++)
    {
      /* 비트가 1이면 픽셀 그리기 */
      if ((glyph[col] & (1U << row)) != 0U)
      {
        EEPROM_Test_SetPixel(x + col, y + row, gray);
      }
    }
  }
}

/*
 * EEPROM_Test_DrawString()
 * 지정한 좌표부터 문자열을 프레임버퍼에 그리기
 * 
 * 동작: 각 문자를 순회하며 DrawChar() 호출, x좌표를 갱신하며 다음 문자 배치
 * 
 * [인자]
 *   x, y  : 문자열 시작 좌표
 *   text  : 널 종료 문자열
 *   gray  : 그레이스케일 값
 */
static void EEPROM_Test_DrawString(uint32_t x, uint32_t y, const char *text, uint8_t gray)
{
  uint32_t cursor_x;

  cursor_x = x;
  while ((text != 0) && (*text != '\0'))
  {
    EEPROM_Test_DrawChar(cursor_x, y, *text, gray);
    /* 다음 문자를 위해 x좌표 update (글자 너비 + 간격) */
    cursor_x += EEPROM_TEST_CHAR_W + EEPROM_TEST_CHAR_SPACING;
    text++;
  }
}

/*
 * EEPROM_Test_DrawHex8()
 * 8비트 값을 16진수 2글자로 출력 (예: 0xAA → "AA")
 * 
 * [인자]
 *   x, y  : 시작 좌표
 *   value : 출력할 8비트 정수값
 *   gray  : 그레이스케일
 */
static void EEPROM_Test_DrawHex8(uint32_t x, uint32_t y, uint8_t value, uint8_t gray)
{
  static const char hex_digits[] = "0123456789ABCDEF";
  char text[3];

  text[0] = hex_digits[(value >> 4) & 0x0F];  /* 상위 4비트 */
  text[1] = hex_digits[value & 0x0F];         /* 하위 4비트 */
  text[2] = '\0';
  EEPROM_Test_DrawString(x, y, text, gray);
}

/*
 * EEPROM_Test_DrawHex16()
 * 16비트 값을 16진수 4글자로 출력 (예: 0x1234 → "1234")
 * 
 * [인자]
 *   x, y  : 시작 좌표
 *   value : 출력할 16비트 정수값
 *   gray  : 그레이스케일
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
 * 32비트 값을 16진수 8글자로 출력 (예: 0x12345678 → "12345678")
 * 
 * [인자]
 *   x, y  : 시작 좌표
 *   value : 출력할 32비트 정수값
 *   gray  : 그레이스케일
 */
static void EEPROM_Test_DrawHex32(uint32_t x, uint32_t y, uint32_t value, uint8_t gray)
{
  static const char hex_digits[] = "0123456789ABCDEF";
  char text[9];
  uint32_t idx;

  /* 가장 하위 니블부터 시작해 역순으로 문자 배치 */
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
 * 테스트 시작 화면 표시
 * 
 * 표시 내용:
 * - "EEPROM TEST" 제목
 * - "PATTERN" 과 테스트 패턴값 (0xAA)
 * - "START" 텍스트
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
  
  /* 프레임버퍼를 OLED에 전송 */
  OLED_SSD1322_Drv_WriteFrame(s_frame);
}

/*
 * EEPROM_Test_ShowProgressScreen()
 * WRITE/VERIFY 진행 중 진행상황 표시 화면
 * 
 * 표시 내용:
 * - "EEPROM TEST" 제목
 * - "PATTERN" 과 테스트 패턴값 (0xAA)
 * - 현재 페이즈 ("WRITE" 또는 "VERIFY")
 * - 현재 주소 (16진수 4글자)
 * 
 * [인자]
 *   phase : "WRITE" 또는 "VERIFY" 문자열
 *   addr  : 현재 진행 중인 EEPROM 주소
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
  
  OLED_SSD1322_Drv_WriteFrame(s_frame);
}

/*
 * EEPROM_Test_ShowPassScreen()
 * 테스트 성공 결과 화면
 * 
 * 표시 내용:
 * - "EEPROM TEST" 제목
 * - "PATTERN" 과 테스트 패턴값
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
  
  OLED_SSD1322_Drv_WriteFrame(s_frame);
}


/*
 * EEPROM_Test_GetPhaseText()
 * 테스트 페이즈 enum 값에 해당하는 텍스트 반환
 * 
 * [인자]
 *   phase : EEPROM_TestPhase_t enum 값
 * 
 * [반환값]
 *   "WRITE", "READ", "VERIFY", 또는 "FAIL"
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
 * 테스트 실패 결과 상세 정보 화면
 * 
 * 표시 내용:
 * - "EEPROM TEST" 제목
 * - "FAIL" 결과
 * - 실패한 페이즈 (WRITE/READ/VERIFY)
 * - 실패 주소 (16진수 4글자)
 * - EXP (예상값, 8비트 16진수)
 * - GOT (실제값, 8비트 16진수)
 * - ERR (에러 코드, 32비트 16진수)
 * 
 * [인자]
 *   phase     : 실패한 페이즈
 *   addr      : 실패한 주소
 *   expected  : 예상했던 바이트 데이터
 *   actual    : 실제 읽은 바이트 데이터
 *   error_code : 드라이버 에러 코드
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
  
  OLED_SSD1322_Drv_WriteFrame(s_frame);
}

/*
 * EEPROM_Test_ShowDriverFailScreen()
 * EEPROM 드라이버 에러 발생 시 보여주는 화면
 * 
 * 예상값 = EEPROM_TEST_PATTERN_VALUE (0xAA)
 * 실제값 = 0xEE (드라이버 에러 표시값)
 * 에러코드 = 드라이버의 마지막 에러값
 * 
 * [인자]
 *   phase : 에러 발생한 페이즈
 *   addr  : 에러 발생 주소
 */
static void EEPROM_Test_ShowDriverFailScreen(EEPROM_TestPhase_t phase, uint16_t addr)
{
  EEPROM_Test_ShowFailScreen(phase,
                             addr,
                             EEPROM_TEST_PATTERN_VALUE,
                             0xEEU,                                      /* 드라이버 에러 표시 */
                             EEPROM_CAT24C256_Drv_GetLastError());       /* 실제 에러 코드 조회 */
}


/* ============================================================================
 * 공개 인터페이스 (Public API)
 * ============================================================================ */

/*
 * EEPROM_Test_App_Init()
 * 
 * EEPROM 테스트 애플리케이션 초기화
 * User_Main_Init()에서 호출됨
 * 
 * 역할:
 * 1. OLED 화면 드라이버 초기화
 * 2. 시작 화면 표시
 * 
 * 호출 순서 (역탑다운):
 * main() 
 *  → User_Main_Init() 
 *    → EEPROM_Test_App_Init()
 *      → OLED_SSD1322_Drv_Init()        [하드웨어 드라이버]
 *      → EEPROM_Test_ShowStartScreen()   [UI 표시]
 */
void EEPROM_Test_App_Init(void)
{
  OLED_SSD1322_Drv_Init();        /* OLED 드라이버 초기화 */
  EEPROM_Test_ShowStartScreen();   /* 시작 화면 표시 */
}

/*
 * EEPROM_Test_App_RunOnce()
 * 
 * EEPROM 테스트 실행 (WRITE → READ & VERIFY → RESULT)
 * User_Main_Run()에서 한 번만 호출됨
 * 
 * 절차:
 * 1) page buffer를 0xAA로 채움
 * 2) 전체 EEPROM에 페이지 단위로 WRITE
 * 3) 다시 READ 후 0xAA와 비교(VERIFY)
 * 4) 에러 시 FAIL 화면, 모두 일치하면 PASS 화면
 */
void EEPROM_Test_App_RunOnce(void)
{
  uint16_t addr;
  uint16_t offset;
  HAL_StatusTypeDef hal_status;

  /* 1. 테스트 데이터 준비 */
  memset(s_page_buffer, EEPROM_TEST_PATTERN_VALUE, sizeof(s_page_buffer));
  HAL_Delay(300U);  /* 하드웨어 안정화 대기 */

  /* PHASE 1: WRITE - EEPROM 전체에 테스트 패턴(0xAA) 기록 */
  for (addr = 0U; addr < EEPROM_CAT24C256_TOTAL_BYTES; addr += EEPROM_CAT24C256_PAGE_SIZE)
  {
    /* 1KB 주기마다 진행상황 화면 업데이트 */
    if ((addr % EEPROM_TEST_PROGRESS_STEP) == 0U)
    {
      EEPROM_Test_ShowProgressScreen("WRITE", addr);
    }

    /* EEPROM Driver 호출: 64바이트 페이지 단위로 기록
      * 주소: addr, 데이터: s_page_buffer (0xAA × 64), 크기: 64바이트 */
    hal_status = EEPROM_CAT24C256_Drv_WritePage(addr, s_page_buffer, EEPROM_CAT24C256_PAGE_SIZE);
    
    /* 에러 처리 */
    if (hal_status != HAL_OK)
    {
      EEPROM_Test_ShowDriverFailScreen(EEPROM_TEST_PHASE_WRITE, addr);
      return;  /* 테스트 종료 */
    }
  }

  /* PHASE 2: READ & VERIFY - EEPROM에서 데이터 읽고 검증 */
  for (addr = 0U; addr < EEPROM_CAT24C256_TOTAL_BYTES; addr += EEPROM_CAT24C256_PAGE_SIZE)
  {
    /* 1KB 주기마다 진행상황 화면 업데이트 */
    if ((addr % EEPROM_TEST_PROGRESS_STEP) == 0U)
    {
      EEPROM_Test_ShowProgressScreen("VERIFY", addr);
    }

    /* EEPROM Driver 호출: addr 주소에서 64바이트 읽기 */
    hal_status = EEPROM_CAT24C256_Drv_Read(addr, s_page_buffer, EEPROM_CAT24C256_PAGE_SIZE);
    
    /* 드라이버 에러 처리 */
    if (hal_status != HAL_OK)
    {
      EEPROM_Test_ShowDriverFailScreen(EEPROM_TEST_PHASE_READ, addr);
      return;  /* 테스트 종료 */
    }

    /* 읽은 데이터 검증: 각 바이트가 0xAA인지 확인 */
    for (offset = 0U; offset < EEPROM_CAT24C256_PAGE_SIZE; offset++)
    {
      if (s_page_buffer[offset] != EEPROM_TEST_PATTERN_VALUE)
      {
        /* 데이터 불일치 → 테스트 실패 */
        EEPROM_Test_ShowFailScreen(EEPROM_TEST_PHASE_VERIFY,
                                   (uint16_t)(addr + offset),           /* 실패한 주소 */
                                   EEPROM_TEST_PATTERN_VALUE,           /* 예상값 (0xAA) */
                                   s_page_buffer[offset],               /* 실제값 */
                                   EEPROM_CAT24C256_Drv_GetLastError()); /* 에러 코드 */
        return;  /* 테스트 종료 */
      }
    }
  }

  /* 모든 검증 완료 → 테스트 성공 */
  EEPROM_Test_ShowPassScreen();
}

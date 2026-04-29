#ifndef __ST7735S_DRV_H__
#define __ST7735S_DRV_H__

#include "main.h"

/* LCD 실제 패널 크기(컨트롤러 기준). Init 단계의 panel-wide clear/tuning에 사용됨. */
#define ST7735S_PANEL_WIDTH          128U
#define ST7735S_PANEL_HEIGHT         97U

/* 컨트롤러 RAM 오프셋. MADCTL=0x08 설정에 따른 고정값. */
#define ST7735S_RAM_OFFSET_X         0U
#define ST7735S_RAM_OFFSET_Y         32U

/* 실제로 안정적으로 보이는 표시 영역입니다. (패널 절대 좌표 기준)
 * Phase 0 캘리브레이션으로 확정된 값입니다. */
#define ST7735S_VIEW_X_MIN           23U
#define ST7735S_VIEW_X_MAX           108U
#define ST7735S_VIEW_Y_MIN           1U
#define ST7735S_VIEW_Y_MAX           96U

/* [LEGACY] 논리 좌표 크기(컨트롤러 픽셀 단위). mono 리팩터 이후는 MONO_* 좌표계를 사용. */
#define ST7735S_LOGICAL_WIDTH        ((ST7735S_VIEW_X_MAX) - (ST7735S_VIEW_X_MIN) + 1U)   /* 86 */
#define ST7735S_LOGICAL_HEIGHT       ((ST7735S_VIEW_Y_MAX) - (ST7735S_VIEW_Y_MIN) + 1U)   /* 96 */

/* ============================================================
 * Mono 256x96 좌표계 (Phase 1)
 *   상위 App 계층은 도표 이 좌표계만 사용한다.
 *   1 RGB565 컨트롤러 픽셀 = 가로로 인접한 mono dot 3개 (B/G/R 서브픽셀).
 *   Phase 0 확정: subpixel 0→B, 1→G, 2→R (MADCTL=0x08 BGR).
 * ============================================================ */

/* 패널 사양 기준 mono dot 해상도(고정). */
#define ST7735S_MONO_WIDTH           256U
#define ST7735S_MONO_HEIGHT          96U
#define ST7735S_MONO_FRAME_BYTES     ((ST7735S_MONO_WIDTH * ST7735S_MONO_HEIGHT) / 8U)   /* 3072 */

/* 실효 가시 영역 mono dot 수.
 * Phase 0 캘리브레이션 결과 패널 사양 100% 활용 가능함이 확인됨. */
#define ST7735S_MONO_USABLE_WIDTH    256U
#define ST7735S_MONO_USABLE_HEIGHT   96U

/* mono_x → 동일 윈도우 내 상대 컨트롤러 픽셀 인덱스. */
#define ST7735S_MONO_TO_CTRL_X(mono_x)   ((uint16_t)((mono_x) / 3U))

/* mono_x 의 서브픽셀 채널(0=B, 1=G, 2=R). */
#define ST7735S_MONO_SUBPX(mono_x)       ((uint8_t)((mono_x) % 3U))

/* mono 좌표 → 컨트롤러 RAM 주소 (4단계 변환의 1-step 압축형).
 *   1) relative_controller_x = mono_x / 3
 *   2) relative_controller_y = mono_y
 *   3) ram_x = RAM_OFFSET_X + VIEW_X_MIN + relative_controller_x
 *   4) ram_y = RAM_OFFSET_Y + VIEW_Y_MIN + relative_controller_y
 * 주의: 범위 클램프는 mono 좌표 진입 시점에서 1회만 수행한다(이중 적용 금지). */
#define ST7735S_MONO_TO_RAM_X(mono_x) \
  ((uint16_t)(ST7735S_RAM_OFFSET_X + ST7735S_VIEW_X_MIN + ST7735S_MONO_TO_CTRL_X(mono_x)))
#define ST7735S_MONO_TO_RAM_Y(mono_y) \
  ((uint16_t)(ST7735S_RAM_OFFSET_Y + ST7735S_VIEW_Y_MIN + (mono_y)))

/* LCD 초기화 */
void ST7735S_Drv_Init(void);

/**
 * @brief  3x5 글자 1개를 mono framebuffer에 그린다. Does NOT flush.
 *         Caller must call ST7735S_Drv_FlushMono*().
 *         좌표는 mono dot 단위. on=1 → 글리프 dot ON, on=0 → 글리프 dot OFF.
 *         배경 dot은 건드리지 않는다(투명). 화면 초기화는 ST7735S_Drv_ClearMonoBuffer()로 별도 수행.
 *         (기존 RGB565 색상 인자 기반 의미는 제거되었음.)
 */
void ST7735S_Drv_DrawChar3x5(uint16_t mono_x, uint16_t mono_y, char ch, uint8_t on);

/**
 * @brief  3x5 문자열을 mono framebuffer에 그린다. Does NOT flush.
 *         Caller must call ST7735S_Drv_FlushMono*().
 *         좌표는 mono dot 단위. on=1 → 글리프 dot ON, on=0 → 글리프 dot OFF.
 *         글자 간 cursor 전진은 4 mono dot(글리프 3 + 공백 1).
 *         (기존 RGB565 색상 인자 기반 의미는 제거되었음.)
 */
void ST7735S_Drv_DrawString3x5(uint16_t mono_x, uint16_t mono_y, const char *text, uint8_t on);

/* ============================================================
 * Mono 256x96 Public API (Phase 4)
 *   - 모든 좌표는 mono dot 단위 (mono_x ∈ [0, ST7735S_MONO_WIDTH-1],
 *     mono_y ∈ [0, ST7735S_MONO_HEIGHT-1]).
 *   - 표준 화면 구성 흐름:
 *       ST7735S_Drv_ClearMonoBuffer(0U);
 *       ST7735S_Drv_DrawMonoDot(...);  // 또는 DrawChar3x5 / DrawString3x5
 *       ST7735S_Drv_FlushMono();       // 한 화면 마지막에 1회만 호출
 * ============================================================ */

/**
 * @brief  framebuffer 전체를 on/off로 채운다. Does NOT flush.
 *         Caller must call ST7735S_Drv_FlushMono*().
 *         일반 화면 구성의 표준 진입점.
 */
void ST7735S_Drv_ClearMonoBuffer(uint8_t on);

/**
 * @brief  framebuffer 전체를 on/off로 채우고 내부에서 즉시 ST7735S_Drv_FlushMono()를 호출한다.
 *         Performs immediate FlushMono internally.
 *         Prefer ClearMonoBuffer for normal screen composition.
 *         (테스트 / 디버그 / 긴급 전체 지움 전용. 화면 구성에 사용하면 깜빡임 발생.)
 */
void ST7735S_Drv_ClearMono(uint8_t on);

/**
 * @brief  단일 mono dot을 framebuffer에 set/clear 한다. Does NOT flush.
 *         Caller must call ST7735S_Drv_FlushMono*().
 *         범위(MONO_USABLE_*) 밖 좌표는 무시한다.
 */
void ST7735S_Drv_DrawMonoDot(uint16_t mono_x, uint16_t mono_y, uint8_t on);

/**
 * @brief  framebuffer 전체를 컨트롤러 RAM에 송신한다.
 *         송신 영역: VIEW_X_MIN..VIEW_X_MAX × VIEW_Y_MIN..VIEW_Y_MAX 컨트롤러 픽셀.
 */
void ST7735S_Drv_FlushMono(void);

/**
 * @brief  framebuffer의 사각 영역만 컨트롤러 RAM에 송신한다.
 *         내부에서 컨트롤러 픽셀 경계로 정렬한다(mono_x0 → 내림, mono_x1 → 올림).
 *         좌표 범위는 호출 시점에 클램프된다.
 */
void ST7735S_Drv_FlushMonoRect(uint16_t mono_x0, uint16_t mono_y0,
                               uint16_t mono_x1, uint16_t mono_y1);

#endif /* __ST7735S_DRV_H__ */

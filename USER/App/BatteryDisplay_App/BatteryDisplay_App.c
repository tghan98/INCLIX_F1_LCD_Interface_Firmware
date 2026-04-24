/**
 * @brief 배터리 레벨을 LCD 텍스트로 표시하는 앱입니다.
 */

#include "BatteryDisplay_App.h"
#include "BatteryMonitor_Interface.h"
#include "ST7735S_Drv.h"

#define BATTERYDISPLAY_LEVEL_INVALID  0xFFFFFFFFU
/* [LEGACY] RGB565 색상 상수. Phase 5 이후 mono on/off API만 사용하므로 실제 참조되지 않음. */
#define BATTERYDISPLAY_TEXT_COLOR     0xFFFFU
/* Phase 6: mono dot 좌표 기준 (좌상단 (0,0)).
 *   기존 logical x → mono x = logical * 3 (subpixel 단위 확장).
 *     TEXT_X 3 → 9, MARK_X 59 → 177.
 *   y는 1:1 (LINE0_Y=16, LINE_STEP=7 그대로 사용). */
#define BATTERYDISPLAY_TEXT_X         9U     /* 기존 logical 3 → mono 9 */
#define BATTERYDISPLAY_MARK_X         177U   /* 기존 logical 59 → mono 177 */
#define BATTERYDISPLAY_LINE0_Y        16U    /* y는 1:1 */
#define BATTERYDISPLAY_LINE_STEP      7U     /* y는 1:1 */

/* 배터리 단계별 표시 문자열 */
static const char* battery_level_text[] = {
  "BAT:CRIT 0/3",  /* 0단계 */
  "BAT:LOW  1/3",  /* 1단계 */
  "BAT:MED  2/3",  /* 2단계 */
  "BAT:HIGH 3/3"   /* 3단계 */
};

static uint32_t s_displayed_level = BATTERYDISPLAY_LEVEL_INVALID;

/**
 * @brief 현재 배터리 단계를 화면에 그립니다.
 * @param level 표시할 배터리 단계값
 * @retval 없음
 */
static void BatteryDisplay_App_RenderLevel(uint32_t level)
{
  uint16_t line_y;
  uint32_t line_index;

  if (level > BATTERY_LEVEL_HIGH)
  {
    return;
  }

  // 1) mono framebuffer를 OFF로 지운다 (Phase 6 표준 흐름). 즉시 flush 안 함.
  ST7735S_Drv_ClearMonoBuffer(0U);

  // 2) 단계별 텍스트를 framebuffer에 그린다 (즉시 flush 안 함).
  for (line_index = 0U; line_index <= BATTERY_LEVEL_HIGH; line_index++)
  {
    line_y = (uint16_t)(BATTERYDISPLAY_LINE0_Y + (line_index * BATTERYDISPLAY_LINE_STEP));
    ST7735S_Drv_DrawString3x5(BATTERYDISPLAY_TEXT_X, line_y, battery_level_text[line_index], 1U);
  }

  // 3) 현재 레벨 표시 마커 '>' 출력.
  line_y = (uint16_t)(BATTERYDISPLAY_LINE0_Y + (level * BATTERYDISPLAY_LINE_STEP));
  ST7735S_Drv_DrawString3x5(BATTERYDISPLAY_MARK_X, line_y, ">", 1U);

  // 4) 한 화면 구성 완료 → LCD로 1회 송신.
  ST7735S_Drv_FlushMono();
}

/**
 * @brief 배터리 표시 앱을 초기화합니다.
 * @param 없음
 * @retval 없음
 */
void BatteryDisplay_App_Init(void)
{
  /* LCD를 초기화합니다. */
  ST7735S_Drv_Init();
  s_displayed_level = BATTERYDISPLAY_LEVEL_INVALID;
}

/**
 * @brief 배터리 이벤트를 소비하여 최신 레벨 기준으로 화면을 갱신합니다.
 * @param 없음
 * @retval 없음
 */
void BatteryDisplay_App_Run(void)
{
  BatteryMonitor_AppEvent_t event;
  uint32_t latest_level = BATTERYDISPLAY_LEVEL_INVALID;
  uint8_t has_pending_event = 0U;

  while (BatteryMonitor_Interface_GetEvent(&event) == 0)
  {
    if (event.level <= BATTERY_LEVEL_HIGH)
    {
      latest_level = event.level;
      has_pending_event = 1U;
    }
  }

  /* 누적된 이벤트는 모두 소비하되, 마지막 유효 레벨만 화면에 반영합니다. */
  if ((has_pending_event != 0U) && (latest_level != s_displayed_level))
  {
    BatteryDisplay_App_RenderLevel(latest_level);
    s_displayed_level = latest_level;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

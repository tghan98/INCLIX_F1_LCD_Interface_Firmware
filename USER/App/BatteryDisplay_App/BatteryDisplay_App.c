/**
 * @brief 배터리 레벨을 LCD 텍스트로 표시하는 앱입니다.
 */

#include "BatteryDisplay_App.h"
#include "BatteryMonitor_Interface.h"
#include "ST7735S_Drv.h"

#define BATTERYDISPLAY_LEVEL_INVALID  0xFFFFFFFFU
#define BATTERYDISPLAY_TEXT_COLOR     0xFFFFU
/* 논리 좌표 기준입니다. (0,0)은 표시 영역의 왼쪽 위) */
#define BATTERYDISPLAY_TEXT_X         3U    /* 기존 패널 좌표 24를 논리 좌표 3으로 보정 */
#define BATTERYDISPLAY_MARK_X         59U   /* 기존 패널 좌표 80을 논리 좌표 59로 보정 */
#define BATTERYDISPLAY_LINE0_Y        16U   /* 기존 패널 좌표 18을 논리 좌표 16으로 보정 */
#define BATTERYDISPLAY_LINE_STEP      7U

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

  if (level >= 4U)
  {
    return;
  }

  ST7735S_Drv_Clear(0x0000U);

  for (line_index = 0U; line_index < 4U; line_index++)
  {
    line_y = (uint16_t)(BATTERYDISPLAY_LINE0_Y + (line_index * BATTERYDISPLAY_LINE_STEP));
    ST7735S_Drv_DrawString3x5(BATTERYDISPLAY_TEXT_X, line_y, battery_level_text[line_index], BATTERYDISPLAY_TEXT_COLOR, 0x0000U);
  }

  line_y = (uint16_t)(BATTERYDISPLAY_LINE0_Y + (level * BATTERYDISPLAY_LINE_STEP));
  ST7735S_Drv_DrawString3x5(BATTERYDISPLAY_MARK_X, line_y, ">", BATTERYDISPLAY_TEXT_COLOR, 0x0000U);
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
 * @brief 배터리 단계를 확인하고, 값이 바뀌면 화면을 다시 그립니다.
 * @param 없음
 * @retval 없음
 */
void BatteryDisplay_App_Run(void)
{
  uint32_t current_level;

  current_level = BatteryMonitor_Interface_GetLevel();

  /* 처음 1회 출력하고, 이후에는 단계가 바뀔 때만 다시 그립니다. */
  if (current_level != s_displayed_level)
  {
    BatteryDisplay_App_RenderLevel(current_level);
    s_displayed_level = current_level;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

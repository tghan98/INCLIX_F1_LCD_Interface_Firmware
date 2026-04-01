/* Battery display app - LCD text display for battery level */

#include "BatteryDisplay_App.h"
#include "BatteryMonitor_Interface.h"
#include "ST7735S_Drv.h"

#define BATTERYDISPLAY_LEVEL_INVALID  0xFFFFFFFFU
#define BATTERYDISPLAY_TEXT_COLOR     0xFFFFU
/* Logical coordinates: (0,0) = top-left of visible area (ST7735S_LOGICAL_WIDTH x ST7735S_LOGICAL_HEIGHT) */
#define BATTERYDISPLAY_TEXT_X         3U    /* was 24 (panel) - VIEW_X_MIN 21 = 3  */
#define BATTERYDISPLAY_MARK_X         59U   /* was 80 (panel) - VIEW_X_MIN 21 = 59 */
#define BATTERYDISPLAY_LINE0_Y        16U   /* was 18 (panel) - VIEW_Y_MIN  2 = 16 */
#define BATTERYDISPLAY_LINE_STEP      7U

/* Battery level text strings */
static const char* battery_level_text[] = {
  " BAT:CRIT 0/3",  /* Level 0 */
  " BAT:LOW  1/3",  /* Level 1 */
  " BAT:MED  2/3",  /* Level 2 */
  " BAT:HIGH 3/3"   /* Level 3 */
};

static uint32_t s_displayed_level = BATTERYDISPLAY_LEVEL_INVALID;

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

void BatteryDisplay_App_Init(void)
{
  /* Initialize LCD display */
  ST7735S_Drv_Init();
  s_displayed_level = BATTERYDISPLAY_LEVEL_INVALID;
}

void BatteryDisplay_App_Run(void)
{
  uint32_t current_level;

  current_level = BatteryMonitor_Interface_GetLevel();

  /* Draw once at startup, then redraw only when the level changes. */
  if (current_level != s_displayed_level)
  {
    BatteryDisplay_App_RenderLevel(current_level);
    s_displayed_level = current_level;
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

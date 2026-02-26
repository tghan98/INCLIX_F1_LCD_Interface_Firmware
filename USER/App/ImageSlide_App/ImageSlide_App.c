#include "ImageSlide_App.h"

#include "ImageSlide_Assets.h"
#include "OLED_SSD1322_Drv.h"
#include "Button_Interface.h"
#include "BatteryMonitor_Interface.h"
#include "string.h"

/* Frame change interval */
#define IMAGE_SLIDE_INTERVAL_MS    1500U

/* Battery icon blocks (5x3 pixels) */
#define BAT_BLOCK_X_START          243U
#define BAT_BLOCK_WIDTH            5U
#define BAT_BLOCK_HEIGHT           3U
#define BAT_BLOCK_L3_Y             16U
#define BAT_BLOCK_L2_Y             20U
#define BAT_BLOCK_L1_Y             24U


static uint32_t s_last_tick;
static uint32_t s_frame_index;
static uint8_t s_is_paused;
static uint8_t s_frame_buffer[OLED_SSD1322_FRAME_BYTES];

/* Helper: Draw battery icon on frame buffer at top-right corner */
static void ImageSlide_DrawBatteryBlock(uint8_t *frame, uint32_t x_start, uint32_t y_start, uint8_t gray)
{
  uint32_t x;
  uint32_t y;
  uint32_t row_offset;
  uint32_t byte_index;
  uint8_t nibble_idx;
  uint8_t nibble;

  nibble = (uint8_t)(gray & 0x0FU);

  for (y = y_start; y < (y_start + BAT_BLOCK_HEIGHT); y++)
  {
    row_offset = y * 128U;
    for (x = x_start; x < (x_start + BAT_BLOCK_WIDTH); x++)
    {
      byte_index = row_offset + (x / 2U);
      nibble_idx = x % 2U;
      /* SSD1322: left pixel uses high nibble, right pixel uses low nibble */
      if (nibble_idx == 0U)
      {
        frame[byte_index] = (frame[byte_index] & 0x0F) | (uint8_t)(nibble << 4);
      }
      else
      {
        frame[byte_index] = (frame[byte_index] & 0xF0) | nibble;
      }
    }
  }
}

static void ImageSlide_DrawBatteryIcon(uint8_t *frame, uint32_t battery_level)
{
  /* Base: three white blocks */
  ImageSlide_DrawBatteryBlock(frame, BAT_BLOCK_X_START, BAT_BLOCK_L3_Y, 0x0FU);
  ImageSlide_DrawBatteryBlock(frame, BAT_BLOCK_X_START, BAT_BLOCK_L2_Y, 0x0FU);
  ImageSlide_DrawBatteryBlock(frame, BAT_BLOCK_X_START, BAT_BLOCK_L1_Y, 0x0FU);

  /* Black out blocks by level: L3=full, L2=2칸, L1=1칸, L0=0칸 */
  if (battery_level == 2U)
  {
    ImageSlide_DrawBatteryBlock(frame, BAT_BLOCK_X_START, BAT_BLOCK_L3_Y, 0x00U);
  }
  else if (battery_level == 1U)
  {
    ImageSlide_DrawBatteryBlock(frame, BAT_BLOCK_X_START, BAT_BLOCK_L3_Y, 0x00U);
    ImageSlide_DrawBatteryBlock(frame, BAT_BLOCK_X_START, BAT_BLOCK_L2_Y, 0x00U);
  }
  else if (battery_level == 0U)
  {
    ImageSlide_DrawBatteryBlock(frame, BAT_BLOCK_X_START, BAT_BLOCK_L3_Y, 0x00U);
    ImageSlide_DrawBatteryBlock(frame, BAT_BLOCK_X_START, BAT_BLOCK_L2_Y, 0x00U);
    ImageSlide_DrawBatteryBlock(frame, BAT_BLOCK_X_START, BAT_BLOCK_L1_Y, 0x00U);
  }
}

void ImageSlide_App_Init(void)
{
  OLED_SSD1322_Drv_Init();

  s_last_tick = HAL_GetTick();
  s_frame_index = 0U;
  s_is_paused = 0U;

  if (g_image_slide_frame_count > 0U)
  {
    /* Copy frame to buffer, draw battery icon, and display */
    memcpy(s_frame_buffer, g_image_slide_frames[s_frame_index], OLED_SSD1322_FRAME_BYTES);
            if (s_frame_index == 2U)
            {
          ImageSlide_DrawBatteryIcon(s_frame_buffer, BatteryMonitor_Interface_GetLevel());
            }
    OLED_SSD1322_Drv_WriteFrame(s_frame_buffer);
  }
}

void ImageSlide_App_Run(void)
{
  uint32_t now;
  ButtonAppEvent_t btn_event;
  BatteryMonitor_AppEvent_t bat_event;

  if (g_image_slide_frame_count == 0U)
  {
    return;
  }

  /* Handle all queued battery events (works in both paused and running states) */
  while (BatteryMonitor_Interface_GetEvent(&bat_event) == 0)
  {
    /* If on asset 2 (battery icon frame), update and display */
        if (s_frame_index == 2U)
        {
      memcpy(s_frame_buffer, g_image_slide_frames[s_frame_index], OLED_SSD1322_FRAME_BYTES);
      ImageSlide_DrawBatteryIcon(s_frame_buffer, bat_event.level);
      OLED_SSD1322_Drv_WriteFrame(s_frame_buffer);
        }
  }

  /* Handle all queued button events */
  while (Button_Interface_GetEvent(&btn_event) == 0)
  {
    if ((btn_event.button_id == BUTTON_ID_PAUSE) && (btn_event.event == BUTTON_EVENT_CLICK))
    {
      s_is_paused = !s_is_paused;

      /* Reset timing on resume so frame does not jump immediately */
      if (s_is_paused == 0U)
      {
        s_last_tick = HAL_GetTick();
      }
    }
  }

  if (s_is_paused != 0U)
  {
    return;
  }

  now = HAL_GetTick();
  if ((now - s_last_tick) < IMAGE_SLIDE_INTERVAL_MS)
  {
    return;
  }

  s_last_tick = now;
  s_frame_index = (s_frame_index + 1U) % g_image_slide_frame_count;
  
  /* Copy frame to buffer, draw battery icon, and display */
  memcpy(s_frame_buffer, g_image_slide_frames[s_frame_index], OLED_SSD1322_FRAME_BYTES);
  if (s_frame_index == 2U)
  {
    ImageSlide_DrawBatteryIcon(s_frame_buffer, BatteryMonitor_Interface_GetLevel());
  }
  OLED_SSD1322_Drv_WriteFrame(s_frame_buffer);
}

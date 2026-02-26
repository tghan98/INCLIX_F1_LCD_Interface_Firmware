#include "ImageSlide_App.h"

#include "ImageSlide_Assets.h"
#include "OLED_SSD1322_Drv.h"
#include "Button_Interface.h"

/* Frame change interval */
#define IMAGE_SLIDE_INTERVAL_MS    1500U

static uint32_t s_last_tick;
static uint32_t s_frame_index;
static uint8_t s_is_paused;

void ImageSlide_App_Init(void)
{
  OLED_SSD1322_Drv_Init();

  s_last_tick = HAL_GetTick();
  s_frame_index = 0U;
  s_is_paused = 0U;

  if (g_image_slide_frame_count > 0U)
  {
    OLED_SSD1322_Drv_WriteFrame(g_image_slide_frames[s_frame_index]);
  }
}

void ImageSlide_App_Run(void)
{
  uint32_t now;
  ButtonAppEvent_t btn_event;

  if (g_image_slide_frame_count == 0U)
  {
    return;
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
  OLED_SSD1322_Drv_WriteFrame(g_image_slide_frames[s_frame_index]);
}

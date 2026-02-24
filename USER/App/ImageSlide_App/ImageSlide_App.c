#include "ImageSlide_App.h"

#include "ImageSlide_Assets.h"
#include "OLED_SSD1322_Drv.h"

#define IMAGE_SLIDE_INTERVAL_MS    1500U

static uint32_t s_last_tick;
static uint32_t s_frame_index;

void ImageSlide_App_Init(void)
{
  OLED_SSD1322_Drv_Init();

  s_last_tick = HAL_GetTick();
  s_frame_index = 0U;

  if (g_image_slide_frame_count > 0U)
  {
    OLED_SSD1322_Drv_WriteFrame(g_image_slide_frames[s_frame_index]);
  }
}

void ImageSlide_App_Run(void)
{
  uint32_t now;

  if (g_image_slide_frame_count == 0U)
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

#include "ImageSlide_App.h"

#include "ImageSlide_Assets.h"
#include "OLED_SSD1322_Drv.h"

/* 이미지 전환 간격 (밀리초) */
#define IMAGE_SLIDE_INTERVAL_MS    1500U

/* 마지막으로 이미지를 전환한 시간 */
static uint32_t s_last_tick;
/* 현재 표시 중인 이미지 번호 */
static uint32_t s_frame_index;

/**
 * @brief 이미지 슬라이드 앱 초기화
 * @note OLED 드라이버를 초기화하고 첫 번째 이미지를 화면에 표시
 */
void ImageSlide_App_Init(void)
{
  /* OLED 화면 초기화 */
  OLED_SSD1322_Drv_Init();

  /* 시작 시간 기록 */
  s_last_tick = HAL_GetTick();
  /* 첫 번째 이미지부터 시작 */
  s_frame_index = 0U;

  /* 이미지가 있으면 첫 번째 이미지를 화면에 표시 */
  if (g_image_slide_frame_count > 0U)
  {
    OLED_SSD1322_Drv_WriteFrame(g_image_slide_frames[s_frame_index]);
  }
}

/**
 * @brief 이미지 슬라이드 앱 실행 (주기적으로 호출 필요)
 * @note 설정된 시간 간격마다 다음 이미지로 자동 전환
 */
void ImageSlide_App_Run(void)
{
  uint32_t now;

  /* 이미지가 없으면 아무것도 하지 않음 */
  if (g_image_slide_frame_count == 0U)
  {
    return;
  }

  /* 현재 시간 가져오기 */
  now = HAL_GetTick();
  /* 아직 전환 시간이 안됐으면 대기 */
  if ((now - s_last_tick) < IMAGE_SLIDE_INTERVAL_MS)
  {
    return;
  }

  /* 전환 시간 업데이트 */
  s_last_tick = now;
  /* 다음 이미지로 넘기기 (마지막 이미지 다음은 첫 이미지) */
  s_frame_index = (s_frame_index + 1U) % g_image_slide_frame_count;
  /* 새로운 이미지를 화면에 표시 */
  OLED_SSD1322_Drv_WriteFrame(g_image_slide_frames[s_frame_index]);
}

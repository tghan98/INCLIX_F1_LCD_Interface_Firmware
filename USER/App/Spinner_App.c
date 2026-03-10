#include "Spinner_App.h"
#include "../Drive/Spinner_Drv.h"
#include "../Drive/Sliding_Icon_Drv.h"
#include "OLED_SSD1322_Drv.h"
#include <string.h>

#define SPINNER_ROTATE_INTERVAL_MS  (17U)

static uint8_t s_frame[OLED_SSD1322_FRAME_BYTES];
static uint32_t s_last_tick = 0U;
static uint32_t s_angle_idx = 0U;
static uint32_t s_prev_draw_time = 0U; // TODO: 테스트 후 삭제
static uint32_t s_prev_send_time = 0U; // TODO: 테스트 후 삭제

int32_t Spinner_App_Init(void)
{
  OLED_SSD1322_Drv_Init();
  Sliding_Icon_Drv_Init();  // Init에서 자동으로 시작됨

  memset(s_frame, 0, sizeof(s_frame));
  Spinner_Drv_DrawRotatingSprite(s_frame, s_angle_idx);
  OLED_SSD1322_Drv_WriteFrame(s_frame);
  s_last_tick = HAL_GetTick();

  return 0;
}

int32_t Spinner_App_Run(void)
{
  uint32_t now;
  uint32_t draw_start, draw_end, send_start, send_end; // TODO: 테스트 후 삭제
  uint32_t draw_time, send_time; // TODO: 테스트 후 삭제

  now = HAL_GetTick();
  if ((now - s_last_tick) < SPINNER_ROTATE_INTERVAL_MS)
  {
    return 0;
  }

  s_last_tick = now;
  s_angle_idx = (s_angle_idx + 1U) % SPINNER_FRAMES;

  // 슬라이딩 아이콘 상태 업데이트
  Sliding_Icon_Drv_Update();

  memset(s_frame, 0, sizeof(s_frame));
  
  // DEBUG: 타이밍 정보 표시 (우측 상단)
  Spinner_Drv_DrawNumber(s_frame, 220, 5, s_prev_draw_time);   /* Draw: X ms */
  Spinner_Drv_DrawNumber(s_frame, 220, 14, s_prev_send_time);  /* Send: X ms */
  Spinner_Drv_DrawNumber(s_frame, 220, 23, SPINNER_FRAMES);    /* Frames: 60 */
  
  // DEBUG: 라벨 표시 (숫자 바로 왼쪽)
  Spinner_Drv_DrawText(s_frame, 195, 5, "Draw");
  Spinner_Drv_DrawText(s_frame, 195, 14, "Send");
  
  // TODO: 테스트 후 삭제 - 그리기 시간 측정
  draw_start = HAL_GetTick();
  Spinner_Drv_DrawRotatingSprite(s_frame, s_angle_idx);
  
  // 슬라이딩 아이콘 그리기 (모래시계 위에 그려짐)
  Sliding_Icon_Drv_Draw(s_frame);
  
  draw_end = HAL_GetTick();
  draw_time = draw_end - draw_start;
  
  // TODO: 테스트 후 삭제 - 송신 시간 측정
  send_start = HAL_GetTick();
  OLED_SSD1322_Drv_WriteFrame(s_frame);
  send_end = HAL_GetTick();
  send_time = send_end - send_start;
  
  // TODO: 테스트 후 삭제 - 다음 프레임을 위해 저장
  s_prev_draw_time = draw_time;
  s_prev_send_time = send_time;

  return 0;
}

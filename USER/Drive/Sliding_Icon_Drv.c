#include "Sliding_Icon_Drv.h"
#include "Spinner_Drv.h"
#include <string.h>

/* Cassette icon bitmap - 23x10 (90° rotated from 카세트.bmp) */
static const uint8_t s_icon_bitmap[SLIDING_ICON_HEIGHT][SLIDING_ICON_WIDTH] = {
  { 0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15, 0},
  {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
  {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
  {15,15,15,15,15, 0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
  {15,15,15,15, 0, 0, 0,15,15,15,15,15, 0, 0, 0, 0, 0, 0, 0,15,15,15,15},
  {15,15,15,15, 0, 0, 0,15,15,15,15,15, 0, 0, 0, 0, 0, 0, 0,15,15,15,15},
  {15,15,15,15,15, 0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
  {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
  {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15},
  { 0,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15, 0}
};

/* Static variables */
static SlidingState_t s_state = SLIDING_STATE_HIDDEN;
static int32_t s_current_x = 0;  // 음수 가능 (화면 밖)
static uint32_t s_stay_counter = 0;
static uint32_t s_hidden_counter = 0;  // 숨김 상태 카운터

/* Screen dimensions */
#define SCREEN_WIDTH  (256)

void Sliding_Icon_Drv_Init(void)
{
  s_state = SLIDING_STATE_SLIDING_IN;  // 시작하자마자 슬라이딩 시작
  s_current_x = -(int32_t)SLIDING_ICON_WIDTH;  // 완전히 왼쪽 밖에서 시작
  s_stay_counter = 0;
  s_hidden_counter = 0;
}

void Sliding_Icon_Drv_Start(void)
{
  if (s_state == SLIDING_STATE_HIDDEN)
  {
    s_state = SLIDING_STATE_SLIDING_IN;
    s_current_x = -(int32_t)SLIDING_ICON_WIDTH;
    s_stay_counter = 0;
  }
}

void Sliding_Icon_Drv_Update(void)
{
  switch (s_state)
  {
    case SLIDING_STATE_SLIDING_IN:
      s_current_x += SLIDING_SPEED_PX_PER_FRAME;
      
      // 목표 위치에 도착하면 머무르기 시작
      if (s_current_x >= (int32_t)SLIDING_TARGET_X_POS)
      {
        s_current_x = (int32_t)SLIDING_TARGET_X_POS;
        s_state = SLIDING_STATE_STAYING;
        s_stay_counter = 0;
      }
      break;
      
    case SLIDING_STATE_STAYING:
      s_stay_counter++;
      if (s_stay_counter >= SLIDING_STAY_FRAMES)
      {
        s_state = SLIDING_STATE_SLIDING_OUT;
      }
      break;
      
    case SLIDING_STATE_SLIDING_OUT:
      // 왔던 길로 되돌아가기 (왼쪽으로)
      s_current_x -= SLIDING_SPEED_PX_PER_FRAME;
      
      // 완전히 왼쪽 밖으로 나가면 숨김
      if (s_current_x <= -(int32_t)SLIDING_ICON_WIDTH)
      {
        s_state = SLIDING_STATE_HIDDEN;
        s_current_x = -(int32_t)SLIDING_ICON_WIDTH;
        s_hidden_counter = 0;  // 숨김 카운터 리셋
      }
      break;
      
    case SLIDING_STATE_HIDDEN:
      // 일정 시간 후 다시 시작 (무한 반복)
      s_hidden_counter++;
      if (s_hidden_counter >= SLIDING_HIDDEN_FRAMES)
      {
        s_state = SLIDING_STATE_SLIDING_IN;
        s_current_x = -(int32_t)SLIDING_ICON_WIDTH;
        s_hidden_counter = 0;
      }
      break;
      
    default:
      break;
  }
}

void Sliding_Icon_Drv_Draw(uint8_t *frame)
{
  uint32_t x, y;
  int32_t screen_x;
  uint8_t gray;
  
  if (s_state == SLIDING_STATE_HIDDEN)
  {
    return;  // 숨겨진 상태면 그리지 않음
  }
  
  // 아이콘의 각 픽셀을 그림
  for (y = 0; y < SLIDING_ICON_HEIGHT; y++)
  {
    for (x = 0; x < SLIDING_ICON_WIDTH; x++)
    {
      screen_x = s_current_x + (int32_t)x;
      
      // 화면 범위 체크
      if (screen_x >= 0 && screen_x < SCREEN_WIDTH)
      {
        gray = s_icon_bitmap[y][x];
        if (gray > 0)  // 투명도 처리 (0은 그리지 않음)
        {
          Spinner_Drv_SetPixel4bpp(frame, (uint32_t)screen_x, SLIDING_ICON_Y_POS + y, gray);
        }
      }
    }
  }
}

SlidingState_t Sliding_Icon_Drv_GetState(void)
{
  return s_state;
}

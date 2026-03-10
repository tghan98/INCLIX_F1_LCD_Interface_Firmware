#ifndef SLIDING_ICON_DRV_H
#define SLIDING_ICON_DRV_H

#include <stdint.h>

/* Sliding icon configuration */
#define SLIDING_ICON_WIDTH        (23)   // 카세트 아이콘 너비 (90도 회전됨)
#define SLIDING_ICON_HEIGHT       (10)   // 카세트 아이콘 높이 (90도 회전됨)
#define SLIDING_ICON_Y_POS        (27)   // 화면 세로 위치
#define SLIDING_SPEED_PX_PER_FRAME (2)   // 한 프레임당 이동 픽셀
#define SLIDING_TARGET_X_POS      (64)   // 전진 목표 위치 (픽셀)
#define SLIDING_STAY_FRAMES       (60)   // 머무르는 프레임 수 (약 1초)
#define SLIDING_HIDDEN_FRAMES     (30)   // 숨김 상태 유지 프레임 수 (재시작 전 대기)

/* Sliding states */
typedef enum {
  SLIDING_STATE_HIDDEN = 0,
  SLIDING_STATE_SLIDING_IN,    // 왼쪽 밖 -> 중앙으로
  SLIDING_STATE_STAYING,       // 중앙에 머무름
  SLIDING_STATE_SLIDING_OUT    // 중앙 -> 왼쪽 밖 (되돌아가기)
} SlidingState_t;

/* Driver functions */
void Sliding_Icon_Drv_Init(void);
void Sliding_Icon_Drv_Update(void);
void Sliding_Icon_Drv_Draw(uint8_t *frame);
void Sliding_Icon_Drv_Start(void);  // 슬라이딩 시작
SlidingState_t Sliding_Icon_Drv_GetState(void);

#endif /* SLIDING_ICON_DRV_H */

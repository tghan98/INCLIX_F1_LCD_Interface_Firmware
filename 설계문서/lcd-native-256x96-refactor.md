# Plan: LCD Native 256x96 Refactor

## Status
- done (Phase 1 완료 2026-04-01, 실기기 확인 완료)

## Goal
- 앱 좌표를 (0,0) 기준으로 단순화하고, native 256x96 활용도를 높인다.

## Key Findings (from analysis)
- DrawPixel은 panel 좌표계(VIEW_X_MIN=21~VIEW_X_MAX=107, VIEW_Y_MIN=2~VIEW_Y_MAX=95)로 클리핑
- 내부 controller 전송은 RAM_OFFSET_X+x, RAM_OFFSET_Y+y (x,y = panel coord)
- WriteFrame은 별도로 FRAME_OFFSET_Y=17 적용 → panel Y 17~80만 사용
- 실측 보정: VIEW_X_MIN/X_MAX를 21/107 -> 23/109로 조정, (0,0) L마커 온전 표시 확인
- BatteryDisplay_App은 panel 좌표 직접 사용 (TEXT_X=24, LINE0_Y=18)
- 최종 가시영역 확정: logical X=0..84, Y=0..93 (VIEW_X_MIN/X_MAX=23/107, VIEW_Y_MIN/Y_MAX=2/95)
  - **4점 테스트 코드 위치**: ST7735S_Drv.c L523~526 (ST7735S_Drv_Clear 내부)
  - **4점 좌표**: 좌상단(0,0), 우상단(84,0), 좌하단(0,93), 우하단(84,93)
  - **4점 수동 조정법**: 해당 라인 숫자를 1씩 변경 후 빌드/다운로드로 위치 확인
- 매 픽셀마다 set_window 호출 → 20 SPI 트랜잭션/문자 (성능 문제)

## Steps
### Phase 1: Logical Coordinate Layer (핵심 목표)
1. ST7735S_Drv.h에 논리 좌표 상수 추가
   - ST7735S_LOGICAL_WIDTH  = VIEW_X_MAX - VIEW_X_MIN = 86U
   - ST7735S_LOGICAL_HEIGHT = VIEW_Y_MAX - VIEW_Y_MIN = 93U
2. ST7735S_Drv.c에 변환 매크로 추가 (static inline or #define)
   - LOGICAL_TO_CTRL_X(lx) = ST7735S_RAM_OFFSET_X + ST7735S_VIEW_X_MIN + (lx)
   - LOGICAL_TO_CTRL_Y(ly) = ST7735S_RAM_OFFSET_Y + ST7735S_VIEW_Y_MIN + (ly)
3. st7735s_draw_pixel() 수정
   - 파라미터를 논리 좌표(0 기준)로 받도록 변경
   - 클리핑: x >= LOGICAL_WIDTH || y >= LOGICAL_HEIGHT
   - 내부에서 LOGICAL_TO_CTRL_X/Y 변환 후 set_window 호출
4. DrawChar3x5, DrawString3x5 파라미터 의미를 논리 좌표로 정렬
   (내부 구현은 draw_pixel 경유하므로 자동 반영)
5. BatteryDisplay_App.c 좌표 업데이트
   - TEXT_X: 24 → 24-21 = 3
   - MARK_X:  80 → 80-21 = 59
   - LINE0_Y: 18 → 18-2  = 16

### Phase 2 (선택적): WriteFrame 오프셋 정렬
- FRAME_OFFSET_Y=17 → VIEW_Y_MIN=2로 맞출지 검토
- WriteFrame 호출 코드가 어디에 있는지 확인 후 결정

## Verification
- BatteryDisplay 화면에서 텍스트 위치 동일 여부 확인
- (0,0)에 픽셀 그리면 왼쪽 상단 가시 영역에 표시되는지 확인
- (LOGICAL_WIDTH-1, LOGICAL_HEIGHT-1) 픽셀이 클리핑 없이 표시되는지 확인

## Risks
- 좌표 전환 시 BatteryDisplay 레이아웃이 틀어질 수 있음 → 계산값 재검증 필요
- WriteFrame과 DrawChar 좌표계 혼용 시 혼란 → 주석/명명으로 명확화 필요

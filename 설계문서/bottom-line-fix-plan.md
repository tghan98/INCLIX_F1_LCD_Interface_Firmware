# Plan: 하단 미갱신 라인 제거

## 현황 분석

### 클리어 윈도우 계산 (현재)
- y_end = RAM_OFFSET_Y + PANEL_HEIGHT - 1 = 32 + 96 - 1 = **127**
- 클리어 RAM Y 범위: 32 ~ 127 (96행)
- VIEW_Y_MAX=95 → RAM Y = 32+95 = **127** (클리어 끝과 동일)

### 핵심 가설
물리 패널이 RAM Y=128(panel Y=96)까지 표시하는데 PANEL_HEIGHT=96 이므로  
마지막 1행(RAM Y=128)이 클리어 윈도우에서 빠져 잔재가 남음.

---

## 단계별 테스트 및 개선 계획

### Phase 0: Clean-up (선결 조건)
- `ST7735S_Drv_Clear()` 임시 테스트 코드(흰색→HAL_Delay(500)→검정) 제거
  → line 496~570 일대의 임시 코드 블록 삭제
- 4점 마커 주석은 그대로 유지 (현재 주석처리 상태)

### Phase 1: Diagnostic — TUNING_PATTERN_MODE로 원인 확인
- `ST7735S_Drv.c` line 11: `ST7735S_TUNING_PATTERN_MODE` 값을 **0 → 1** 로 변경
- 빌드 후 전원 인가, 하단 잔재 라인이 여전히 보이는지 관찰
  - **보인다** → PANEL_HEIGHT 부족으로 물리 최하단 행이 클리어되지 않는 것 확정
  - **안 보인다** → 임시 테스트 코드나 init flow 문제 (예상 외)
- 관찰 완료 후 `TUNING_PATTERN_MODE = 0` 으로 즉시 원복

### Phase 2: Fix — PANEL_HEIGHT 1씩 증가 (Phase 1 결과 = 보임인 경우)
| 시도 | PANEL_HEIGHT | y_end (RAM) | 예상 결과 |
|------|-------------|-------------|-----------|
| 현재 | 96 | 127 | 잔재 있음 |
| 1차 | **97** | **128** | 잔재 없을 가능성 높음 |
| 2차 | **98** | **129** | 1차에서도 남아 있을 경우 |

- 수정 위치 2곳 (필수):
  - `ST7735S_Drv.c` line 5 — `#define ST7735S_PANEL_HEIGHT`
  - `ST7735S_Drv.h` line 13 — `#define ST7735S_PANEL_HEIGHT`

### Phase 3: VIEW_Y_MAX 검토 (Phase 2 완료 후 선택적)
- PANEL_HEIGHT 확정 후 물리적으로 표시되는 최하단 row 번호 확인
- VIEW_Y_MAX=95는 가시영역 실측 기반이므로, 잔재가 없어졌는지만 확인하면 충분
- 만약 콘텐츠가 잘리는 문제 없으면 VIEW/LOGICAL 상수는 그대로 유지

### Phase 4: 최종 검증
1. 전원 인가 후 하단 자글거리는 선 사라짐 확인 (육안)
2. BatteryDisplay_App 텍스트 4줄 콘텐츠 정상 표시 확인 (잘리지 않음)
3. ST7735S_Drv_Clear(0x0000U) 호출 후 화면 전체 검정, 잔재 없음 확인

---

## 관련 파일
- [ST7735S_Drv.c](c:\Users\hansu\Documents\STM32_iar_Workspace\INCLIX_F1_LCD_Interface_Firmware_feature_st7735s_replace_ssd1322\USER\Drive\ST7735S_Drv\ST7735S_Drv.c)  
  — line 5 `PANEL_HEIGHT`, line 11 `TUNING_PATTERN_MODE`, line 122 `st7735s_clear_black()`, line 496 `ST7735S_Drv_Clear()`  
- [ST7735S_Drv.h](c:\Users\hansu\Documents\STM32_iar_Workspace\INCLIX_F1_LCD_Interface_Firmware_feature_st7735s_replace_ssd1322\USER\Drive\ST7735S_Drv\ST7735S_Drv.h)  
  — line 13 `PANEL_HEIGHT`

---

## 결정사항
- PANEL_HEIGHT는 .c/h 두 곳 모두 동시에 수정
- TUNING_PATTERN_MODE는 diagnostic 후 반드시 0 원복
- Clear 임시 테스트 코드 제거 (Phase 0)는 모든 단계 전 선행

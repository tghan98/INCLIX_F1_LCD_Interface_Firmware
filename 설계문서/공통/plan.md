## Plan: 4bpp Native 128x97 전환

현재 레거시 256x64 입력 경로를 제거하고, 내부 4bpp 입력 포맷을 128x97로 고정한다. 목적은 다운샘플 평균 로직 제거, 좌우 해상도 손실 제거, RAM 절약 유지(16bpp 내부버퍼 미사용)다.

**Steps**
1. Phase 1 - API/상수 전환
2. ST7735S_Drv.h에서 ST7735S_DRV_WIDTH/HEIGHT/FRAME_BYTES를 128x97 기준으로 변경한다.
3. ST7735S_Drv.h와 ST7735S_Drv.c의 주석을 256x64 호환 문구에서 128x97 네이티브 4bpp 입력 문구로 정리한다.
4. ST7735S_Drv_WriteFrame의 함수 계약(입력 버퍼 크기, 픽셀 매핑 규칙)을 헤더 주석에 명시한다.
5. Phase 2 - WriteFrame 경로 정리 (*depends on 1*)
6. ST7735S_Drv.c에서 nibble 평균 기반 256->128 다운샘플 로직(gray_left/gray_right/gray_mid)을 제거한다.
7. ST7735S_Drv.c에서 입력 stride를 128x97 4bpp 기준(가로 64바이트)으로 재계산하고, 각 픽셀을 직접 RGB565로 변환해 전송한다.
8. ST7735S_Drv.c에서 y 루프와 window 높이를 97줄 기준으로 맞춘다.
9. ST7735S_Drv.c에서 ST7735S_FRAME_OFFSET_Y가 97줄 전송 시 패널 유효 구간과 충돌 없는지 점검하고 필요 시 최소 보정한다.
10. Phase 3 - 레거시 제거 영향 정리 (*depends on 2*)
11. 프로젝트 내 ST7735S_Drv_WriteFrame 호출부를 재검색해 256x64 버퍼 생성 코드가 남아있지 않은지 확인한다.
12. 레거시 입력 전제 주석/문구를 제거하고, 기존 설계문서에 전환 사실과 버퍼 크기 변경점을 기록한다.
13. Phase 4 - 검증 (*depends on 3*)
14. 정적 검증: ST7735S_DRV_FRAME_BYTES가 6272 bytes로 계산되는지 확인한다.
15. 기능 검증: 128x97 테스트 프레임(그라데이션/체커/경계선)으로 깨짐, 줄 밀림, 하단 잘림 여부를 확인한다.
16. 회귀 검증: DrawChar3x5, DrawString3x5, Clear 화면이 기존과 동일하게 동작하는지 확인한다.
17. 성능 검증: 프레임 갱신 주기에서 CPU 부하와 SPI 전송 시간이 기존 대비 악화되지 않는지 비교한다.

**Relevant files**
- c:/Users/hansu/Documents/STM32_iar_Workspace/INCLIX_F1_LCD_Interface_Firmware_feature_st7735s_replace_ssd1322/USER/Drive/ST7735S_Drv/ST7735S_Drv.h - 드라이버 입력 해상도/버퍼 크기 상수와 API 계약 변경
- c:/Users/hansu/Documents/STM32_iar_Workspace/INCLIX_F1_LCD_Interface_Firmware_feature_st7735s_replace_ssd1322/USER/Drive/ST7735S_Drv/ST7735S_Drv.c - WriteFrame 다운샘플 제거 및 128x97 4bpp 직접 변환 전송
- c:/Users/hansu/Documents/STM32_iar_Workspace/INCLIX_F1_LCD_Interface_Firmware_feature_st7735s_replace_ssd1322/USER/App/BatteryDisplay_App/BatteryDisplay_App.c - 프레임 API 미사용 확인(회귀 참조 파일)
- c:/Users/hansu/Documents/STM32_iar_Workspace/IDDD_FW_FUNC__non_OS/설계문서 - 전환 이력 기록(요청 시)

**Verification**
1. 코드 검색: ST7735S_Drv_WriteFrame 호출부와 ST7735S_DRV_WIDTH/HEIGHT 의존부를 전역 검색해 누락 수정이 없는지 확인
2. 빌드: IAR 프로젝트 클린 빌드 후 경고/오류 확인
3. 실기기: 128x97 프레임 패턴 출력 후 전 영역 표시 확인(상/하/좌/우 경계선 포함)
4. 회귀: 텍스트 렌더링 및 Clear 호출 시 기존 동작 동일성 확인
5. 메모리: map 파일에서 RAM 사용량 변화를 확인해 목표 범위 내인지 점검

**Decisions**
- 레거시 256x64 입력 경로는 즉시 제거한다.
- 신규 내부 4bpp 입력 포맷은 128x97로 고정한다.
- 내부 버퍼는 16bpp로 전환하지 않고 4bpp 유지한다.

**Further Considerations**
1. ST7735S_FRAME_OFFSET_Y를 유지할지, 97줄 전체 표시 중심으로 재보정할지 하드웨어 화면 검증 결과로 최종 확정한다.
2. 향후 컬러 UI가 필요해지면 8bpp 인덱스+LUT 또는 부분 타일 RGB565 방식을 후속 검토한다.
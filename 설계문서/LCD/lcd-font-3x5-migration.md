# Plan: LCD Font 3x5 Migration

## Status
- done

## Scope
- ST7735S 드라이버 텍스트 출력 API를 5x7에서 3x5로 전환
- BatteryDisplay 앱 호출부를 3x5로 교체

## Completed
1. 5x7 API 제거, 3x5 API 추가
2. 3x5 glyph 테이블 및 DrawChar/DrawString 구현
3. BatteryDisplay 텍스트 호출부 전환
4. 테스트용 1px 이동 실험 후 원복 완료

## Verification
- 텍스트 축소 출력 정상 확인 (실기기)
- 레벨 선택 마커 출력 정상

## Notes
- 세로획 두께 체감 이슈는 패널 서브픽셀 구조 영향 가능성 높음

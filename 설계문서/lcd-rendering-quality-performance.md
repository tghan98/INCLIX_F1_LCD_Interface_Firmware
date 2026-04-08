# Plan: LCD Rendering Quality & Performance

## Status
- in-progress (DrawChar3x5 셀 버퍼 개선 완료 2026-04-01, 실기기 표시 동일 확인 완료)

## Goal
- 다운샘플 품질 저하와 픽셀 단위 전송 병목을 완화한다.

## Steps
1. 현재 gray_mid 평균 방식과 대체 필터(nearest/weighted) 비교
2. DrawChar/DrawString를 셀 단위 버퍼 전송으로 개선
3. 반복 set_window 호출 최소화 (라인/블록 단위 전송)
4. 측정 지표 정의: 프레임 전송 시간, SPI 호출 수, CPU 점유
5. 최종 품질/성능 트레이드오프 확정

## Verification
- 동일 패턴에서 가독성 비교 촬영
- 문자열 렌더링 시간 전/후 비교
- SPI 트랜잭션 수 로그 비교

## Risks
- 과도한 최적화 시 코드 복잡도 증가
- 품질 개선 필터가 성능을 악화시킬 수 있음

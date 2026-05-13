# 2026-05-12_01_F100_AnalysisEngineTrial_1차완료

## 1. 작업 목적
F100 분석엔진 1차 Trial 합의문서를 기준으로, F1 펌웨어에서 READY_TO_INCUBATE 이후 INCUBATION -> MEASURING -> CALCULATING -> RESULT_DISPLAY까지 실제 계산 흐름이 동작하도록 모듈 추가, 상태 전이 확장, 화면 표시 연결, IAR 빌드 반영을 완료한다. 본 단계의 목표는 테스트 RawData와 임시 LotParam을 사용해 RESULT 화면에 band 결과 문자열이 표시되는 수준까지를 안정화하는 것이다.

## 2. 수행내용

### 2.1 AnalysisEngine 타입 및 테스트 입력 구조 추가
AnalysisEngine_Types를 통해 RawSample_t, AnalysisCalc_t, AnalysisScore_t 및 Lot/Result 구조체를 고정하고, Trial 입력 경로로 AnalysisEngine_TestData를 추가하였다. 이 단계에서 분석엔진 입력 계약을 명확히 분리해 CodeChip 데이터 출처와 측정 데이터 출처가 바뀌어도 AnalysisEngine 호출 시그니처는 유지되도록 구조를 정리하였다.

### 2.2 AnalysisEngine 정성 코어 로직 구현
F100 원본의 정성 분석 흐름을 정수 기반 single-pass 정책으로 축약 구현하였다. 인접 차분, peak 탐색, baseline 계산, 양수 누적 score, cutoff 비교를 거쳐 band decision과 result_text를 생성하도록 구성했으며, 분모 0 및 인덱스 범위 가드를 포함해 M0+ 환경에서 안전하게 동작하도록 반영하였다.

### 2.3 CodeChip 임시 LotParam 공급 경로 추가
CodeChip_App 하위에 CodeChip_TempLotData를 추가하고 CodeChip_Interface_GetLotParam 경로를 연결하였다. VALIDATE_LOT 이후 SequenceManager가 현재 lot 파라미터를 캐시해 CALCULATING에서 재사용하도록 기반을 마련하여, 향후 실제 CodeChip parser로 교체할 때 분석엔진 호출부를 건드리지 않도록 의존 방향을 정리하였다.

### 2.4 SequenceManager 계산 상태/명령 확장 및 결과 보관
SequenceManager에 INCUBATION 상태와 INCUBATION_START/MEASURE_START 명령을 추가하고, CALCULATING 상태에서 AnalysisEngine을 1회 실행하는 정책을 반영하였다. 또한 마지막 계산 결과를 내부 정적 버퍼에 보관하고 Getter를 통해 ScreenManager가 조회할 수 있도록 연결하여 RESULT 표시의 데이터 소유권을 SequenceManager에 고정하였다.

### 2.5 InputInterpreter 및 ScreenManager 결과 화면 연결
READY_TO_INCUBATE 상태의 START 입력을 INCUBATION_START로 변환하도록 InputInterpreter를 수정하고, ScreenManager에 INCUBATION 장면 및 RESULT_DISPLAY 결과 문자열 렌더링을 추가하였다. 기존 RESULT READY 단일 문구를 제거하고 SequenceManager 결과의 result_text를 라인 단위로 출력하도록 변경해 상태 흐름과 화면 출력이 일치하도록 맞췄다.

### 2.6 IAR 프로젝트 반영 및 E2E 빌드 경로 정상화
IAR .ewp에 USE_TEST_RAW_DATA 전처리 심볼, AnalysisEngine_App include path, AnalysisEngine_App/CodeChip_TempLotData 소스 등록을 반영하였다. 해당 반영으로 CALCULATING 구간의 AnalysisEngine 호출 경로가 실제 빌드에 포함되도록 수정하여 RESULT 화면에서 title만 보이고 결과가 비어 있던 문제를 해소하였다.

## 3. 검증 결과
- [x] IAR Rebuild 기준 빌드/다운로드 정상 완료
- [x] STANDBY -> INSERT CODECHIP -> VALIDATE_LOT -> INSERT CASSETTE -> READY_TO_INCUBATE -> INCUBATION -> MEASURING -> CALCULATING -> RESULT_DISPLAY 상태 흐름 확인
- [x] RESULT 화면에서 title만 표시되는 증상 해소 및 C/A/B 라인 표시 확인
- [x] .ewp에 USE_TEST_RAW_DATA 및 AnalysisEngine/TempLotData 파일 등록 반영 확인
- [ ] RESULT 각 라인의 +/- 값 일관성(반복 측정 기준) 최종 검증

## 4. 잔여 이슈/TODO
- 실제 CodeChip 포맷 확정 후 TempLotData 경로를 parser 기반 실데이터 경로로 교체 필요
- 광학 보드 도입 후 AnalysisEngine_TestData를 OpticalSampler 기반 실측 데이터 경로로 교체 필요
- center 3-pass 채택 여부와 정확도/시간 트레이드오프 재검토 필요
- coef_x1000/offset_score를 포함한 보정식 도입 시 int64_t 임시 승격 포함 오버플로 대비 검토 필요
- CALCULATING 1회 실행 시간이 100ms를 초과할 경우 baseline 나눗셈 회피 최적화 적용 여부 검토 필요
- ERROR 원인 세분화 및 RESET 입력원 정책 정리가 필요
- BLE 결과 송신 경로 설계가 필요

## 5. 다음 단계
- RESULT 화면 이후 Cassette 제거 안내 및 STANDBY 복귀 UX를 다음 브랜치 범위로 설계한다.
- 반복 시나리오 기준으로 C/A/B 라인의 +/- 재현성 검증 항목을 정리하고 실측한다.
- 실제 CodeChip parser/OpticalSampler 도입을 위한 인터페이스 경계와 교체 순서를 설계문서로 분리한다.
- 정량 분석 도입 전 fixed-point와 float 후보를 성능/정확도 관점에서 비교 계획으로 정리한다.

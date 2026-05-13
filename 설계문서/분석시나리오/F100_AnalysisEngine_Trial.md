# 2026-05-12_01_F100_AnalysisEngine_Trial_합의계획

## 1. 문서 목적
본 문서는 INCLIX F-1 신제품 펌웨어에서 구버전 INCLIX F100의 분석엔진을 테스트 분석엔진으로 활용하기 위한 1차 Trial 포팅의 설계 합의 사항을 정리한다. 본 문서 범위는 구현 착수 전 설계 확정이며, 실제 코드 작성과 후속 정량 분석/QC/BLE 연동은 다루지 않는다.

## 2. 배경
현재 feature/analysis_prepare_stub_flow 브랜치에서 STANDBY → INSERT CODECHIP → VALIDATE_LOT → INSERT CASSETTE → READY_TO_INCUBATE 까지의 흐름이 Stub 기반으로 동작한다. READY_TO_INCUBATE 이후의 INCUBATION → MEASURING → CALCULATING → RESULT_DISPLAY 구간은 상태만 존재하고 실제 분석 연결이 없다.

후속 브랜치 feature/f100_analysis_engine_trial에서는 F100의 strip 분석 핵심 정성 로직을 F1용 C 모듈로 축약 포팅하여, 테스트 RawData와 임시 LotParam을 사용해 LCD에 실제 정성 결과(POS/NEG)를 표시하는 1차 Trial을 완료한다.

현실 제약은 다음과 같다.
- 실물 Cassette 없음 → 버튼 입력으로 삽입 대체 유지
- CodeChip 데이터 포맷 미정 → 임시 LotParam 상수로 대체
- 광학 보드 미구현 → 측정 raw data를 코드 상수 배열로 대체
- BLE 모듈 없음 → 결과 송신 경로 보류

## 3. 브랜치 방향
브랜치명 후보: feature/f100_analysis_engine_trial

이번 브랜치의 종료 목표 상태는 RESULT_DISPLAY이며, LCD에 정성 분석 결과(축약 표기 기준 `C:+ A:- B:+`)가 표시되는 시점까지를 범위로 한다.

## 4. 합의된 핵심 설계
### 4.1 분석엔진 독립 원칙
AnalysisEngine은 입력 출처를 알지 않는다. 즉 CodeChip이 실제 EEPROM인지 임시 상수인지, raw data가 실제 광학 측정값인지 테스트 배열인지 모르도록 분리한다. 호출 시그니처는 다음 형태로 합의한다.

`AnalysisEngine_Run(const AnalysisLotParam_t* lot, const RawSample_t* raw, uint16_t raw_count, AnalysisResult_t* out_result)`

이 시그니처를 유지하는 한, 추후 실제 CodeChip 포맷이 정해지면 CodeChip_Parser만 교체하고, 실제 광학 보드가 도입되면 AnalysisEngine_TestData 대신 OpticalSampler가 동일 raw 배열을 채워주는 형태로 교체된다.

### 4.2 정성 분석 우선, 정량 분석 제외
1차 Trial 범위는 F100 ImageAnalysis::analysisStrip의 eQuality 분기에 해당하는 정성 분석만 포함한다. 정량(eQuant), applyCalibration, getSCoValue, 4PL/5PL 함수, ratioAB 후처리, QC, InstrumentCheck, PD check 경로는 제외한다.

핵심 알고리즘 단계는 다음과 같다.
1. 인접 차분 배열 계산: `data1[i] = src[i+3] - src[i]`
2. center ± range ± peak_check 범위 내 peak 탐색
3. peak 기준 baseline_width로 base_start/base_end 결정 후 직선 baseline 계산
4. raw - baseline 양수 누적합으로 score 계산
5. control band score > control_cutoff_score 이면 kit_valid
6. 정성 판정
  - 1차 Trial 판정식: `adjusted_score = score`로 두고, `score >= cutoff_score` 이면 POS, 아니면 NEG
  - F100 원본 보정 판정식(후속 fixed-point 단계 검토): `score * coef_x1000 / 1000 + offset_score >= cutoff_score`
  - `coef_x1000`과 `offset_score`는 1차 Trial에서 계산에 적용하지 않고, 후속 보정 단계를 위한 예약 필드/TODO로만 유지한다.

### 4.3 정수 기반 계산 정책 (Trial 1차)
1차 Trial에서는 float와 double을 일절 사용하지 않고 정수 기반 계산만 사용한다. 근거는 다음과 같다.
- 타깃 MCU(STM32G0, Cortex-M0+)는 FPU가 없어 float/double 연산이 소프트웨어 에뮬레이션으로 처리되며, 누적 비용이 크다.
- 정성 분석은 score 누적합과 cutoff 비교만으로 판정 가능하며, baseline 직선 계수도 정수 분자/분모 형태로 유지한 뒤 비교 시점에만 정규화하면 충분하다.
- 정량 결과 표시는 이번 Trial 범위에서 제외되므로 단정도 부동소수점이 필요하지 않다.

구현 정책:
- 의미 기반 타입 alias를 사용한다.

```c
typedef uint16_t RawSample_t;
typedef int32_t  AnalysisCalc_t;
typedef uint32_t AnalysisScore_t;
```

- raw sample은 `RawSample_t`를 사용한다.
- diff, baseline numerator, height 등 음수가 가능한 중간 계산은 `AnalysisCalc_t`를 사용한다.
- score 누적은 `AnalysisScore_t`를 사용한다.
- 중간 곱셈/누적에서 오버플로 위험이 있는 구간은 필요 시 `int64_t` 임시 승격을 허용한다.
- 1차 Trial baseline 계산은 정수 나눗셈을 허용한다.

```text
baseline_y =
    base_start_y
    + ((base_end_y - base_start_y) * (x - base_start_x))
      / (base_end_x - base_start_x);
```

- `(base_end_x - base_start_x) == 0` 가드는 필수로 둔다.
- 1차 Trial에서는 나눗셈 회피 최적화를 적용하지 않는다.
- CALCULATING 1회 실행 시간이 100 ms를 초과할 경우, 나눗셈 회피 최적화(분모 보존 비교)를 후속 단계 TODO로 검토한다.
- `cutoff_score`, `control_cutoff_score`는 `int32_t`로 `AnalysisBandParam_t`에 정의하고 1차 Trial 판정에서 그대로 사용한다. 명칭에 `_score`를 붙여 "scaled 값이 아닌 raw score와 비교한다"는 의미를 필드명에 담는다.
- `coef_x1000`, `offset_score`는 `AnalysisBandParam_t`에 필드만 예약한다. 1차 Trial에서는 참조하지 않으며, 후속 정량 보정 단계에서 도입한다.
- 분모 0 가드, 인덱스 음수/오버런 가드를 모든 진입점에 둔다.
- 후속 단계에서 정량 분석을 도입할 시 별도 분기에서 fixed-point(Q15/Q31) 또는 float 도입을 재검토한다. 1차 Trial에서는 도입하지 않는다.

### 4.4 Single-pass 정책 (Trial 1차)
F100 원본은 center 위치를 0/+5/+10으로 옮겨가며 3회 분석 후 score가 최대인 패스를 채택한다. 1차 Trial에서는 no=0 단일 패스만 실행한다. 근거는 다음과 같다.
- M0+ 환경에서 3-pass는 분석 시간이 3배가 되며, User_Main_Run 폴링 주기에 영향을 줄 수 있다.
- 1차 Trial은 “정성 결과가 LCD에 표시되는 흐름 검증”이 목적이며, peak 위치 보정 정확도 비교는 후속 단계에서 다룬다.

구현 정책:
- analysisStrip 축약 함수는 single-pass로 구현한다.
- center shift 다중 패스는 후속 단계 TODO로 주석만 남긴다.

### 4.5 데이터 구조 owner 정책
분석에 사용되는 공통 데이터 구조 `AnalysisLotParam_t`, `AnalysisBandParam_t`, `AnalysisResult_t`의 정의 owner는 AnalysisEngine_App 모듈로 한다. 정의는 `AnalysisEngine_Types.h` 한 파일에만 둔다. 근거는 다음과 같다.
- LotParam은 분석엔진의 입력 계약이며, CodeChip은 그 계약을 채워주는 producer 역할이다.
- 의존 방향은 `CodeChip_App` → `AnalysisEngine_Types.h` 단방향이며 순환 의존이 발생하지 않는다.
- `AnalysisEngine_Types.h`에는 POD 구조체와 enum, 매크로만 두고 함수 프로토타입은 두지 않는다. 이로써 CodeChip_App 빌드가 AnalysisEngine 구현 파일에 묶이지 않는다.

CodeChip 측에서는 `CodeChip_TempLotData.{h,c}`를 `USER/App/CodeChip_App/` 하위에 두고, `CodeChip_Interface_GetLotParam()` 형태의 getter로 SequenceManager에 LotParam을 제공한다. 임시 데이터 출처가 CodeChip_App 안에 있어야, 후속 단계에서 실제 read/parse 모듈로 교체할 때 AnalysisEngine 측을 건드리지 않는다.

VALIDATE_LOT 성공 시 SequenceManager는 `CodeChip_Interface_GetLotParam()`으로 LotParam을 받아 `s_current_lot_param`에 복사해 현재 검사 컨텍스트로 고정한다. CALCULATING 단계에서는 CodeChip을 다시 조회하지 않고, 캐시된 `s_current_lot_param`을 `AnalysisEngine_Run()`에 전달한다.

### 4.6 결과 보관 위치
1차 Trial에서는 분석 결과 `AnalysisResult_t`를 SequenceManager_App 내부 `static AnalysisResult_t s_last_result;` 단일 인스턴스로 보관한다. 근거는 다음과 같다.
- 결과 1세트, 단일 검사 사이클, 동기 흐름이라는 현재 제약상 추가 모듈 도입이 불필요하다.
- 향후 BLE 송신, 이력 저장, QC 판정 등 결과 소비자가 늘어날 경우 별도 `AnalysisContext_App`으로 분리한다.

ScreenManager는 `SequenceManager_Interface_GetLastResult(const AnalysisResult_t** out)` 형태의 const 포인터 getter로 결과를 조회한다.

### 4.7 결과 표시 책임 분담
AnalysisResult에는 band별 decision enum을 필수 필드로 포함한다.

```c
typedef enum
{
  ANALYSIS_DECISION_INVALID = 0,
  ANALYSIS_DECISION_NEGATIVE,
  ANALYSIS_DECISION_POSITIVE
} AnalysisDecision_t;
```

AnalysisEngine은 1차 Trial부터 decision enum을 필수 출력값으로 채운다. `result_text[i][12]`는 LCD 표시 편의를 위한 보조 필드로 함께 유지한다.

`result_text` 포맷 제약:
- AnalysisEngine은 `result_text[i]`에 단일 표준 포맷 한 가지만 채운다.
- 포맷: `"{band_name}:{+/-}"` (예: `"COV:+"`, `"A:-"`, `"B:+"`)
- AnalysisEngine 내부에서 화면 크기 기반 포맷 분기를 하지 않는다.
- ScreenManager는 1차 Trial에서 `result_text`를 그대로 출력한다.
- ST7735S 픽셀 제약으로 추가 축약이 필요하면 ScreenManager 측에서 truncate하거나 별도 ResultFormatter를 두는 방향으로 대응한다.
- BLE 송신, 이력 저장 등 후속 소비자는 decision enum만 사용하고 `result_text`는 무시한다.
- 후속 단계에서 decision enum 기반 표시 책임 분리를 검토한다.

### 4.8 SequenceManager 상태/명령 확장
상태 추가:
- INCUBATION

명령 추가:
- INCUBATION_START
- MEASURE_START

전이 정책:
- READY_TO_INCUBATE + INCUBATION_START → INCUBATION
- INCUBATION에서 일정 시간 경과 후 자체 전이 → MEASURING
- MEASURING에서 테스트 RawData 준비 후 자체 전이 → CALCULATING
- CALCULATING 진입 시 AnalysisEngine_Run() 1회 실행 후 CALCULATION_DONE → RESULT_DISPLAY

CALCULATING 1회 실행 보장은 VALIDATE_LOT의 `s_lot_validation_done` 패턴을 답습하여 `s_calculation_done` 플래그로 처리하며, CALCULATING 상태 진입 시점에 리셋한다.

### 4.9 InputInterpreter 정책
READY_TO_INCUBATE 상태에서 START_REQUEST를 무시하는 기존 가드는 유지한다. 대신 READY_TO_INCUBATE에서의 버튼 클릭은 신규 명령 `INCUBATION_START`로 변환하여 SequenceManager에 전달한다. 기존 분석 준비 흐름의 안전장치(POWER 대기 시 pending 처리 등)와 충돌하지 않도록 START_REQUEST 경로와 분리한다.

### 4.10 ScreenManager 장면 확장
다음 화면 상태를 추가한다.
- INCUBATION
- (MEASURING / CALCULATING / RESULT_DISPLAY는 기존 enum 활용)

RESULT_DISPLAY는 단순 “RESULT READY” 표기를 제거하고 `s_last_result.result_text[i]` 문자열을 렌더링한다.

## 5. 모듈 분리 합의
신규 추가 모듈 위치는 다음과 같다.

```
USER/App/AnalysisEngine_App/
  AnalysisEngine_Types.h
  AnalysisEngine_App.h
  AnalysisEngine_App.c
  AnalysisEngine_TestData.h
  AnalysisEngine_TestData.c

USER/App/CodeChip_App/
  CodeChip_TempLotData.h
  CodeChip_TempLotData.c
```

AnalysisEngine_App에는 별도 `AnalysisEngine_Interface.{h,c}` 계층을 두지 않는다. AnalysisEngine은 User_Main_Run 폴링 대상이 아니라 SequenceManager가 1회 호출하는 순수 함수 모듈이며, Interface 계층은 폴링 모듈에 한정해서 컨벤션을 유지한다.

`AnalysisEngine_TestData.{h,c}`는 컴파일 옵션(`USE_TEST_RAW_DATA`)으로 감싸서 광학 보드 도입 시 빌드에서 제거 가능한 구조로 둔다.

AnalysisEngine_TestData는 1차 Trial에서 테스트 RawData만 제공하며, API 후보는 다음과 같다.

```c
int32_t AnalysisEngine_TestData_GetRawData(
  const RawSample_t** out_raw,
  uint16_t* out_count
);
```

1차 Trial에서는 const 포인터 반환 방식으로 진행하고, 광학 보드 도입 후에는 OpticalSampler가 동일 역할을 대체하도록 TODO로 남긴다.

## 6. F100 포팅 시 위험 관리
원본 `ImageAnalysis::analysisStrip` 코드의 다음 항목을 위험으로 식별하고 다음과 같이 대응한다.

- VLA(`double data1[srcData.size() - 3];`) → `LEN_SCAN_DATA`를 매크로 상수로 고정하고 정적 배열로 치환한다.
- double 사용 → 4.3 정수 기반 계산 정책에 따라 `RawSample_t`/`AnalysisCalc_t`/`AnalysisScore_t` 조합으로 치환한다.
- 3-pass 시프트 루프 → 4.4 single-pass 정책에 따라 1-pass로 치환한다.
- 인덱스 음수/오버런 → start/end 모두 `0 <= start && end < raw_count` 범위 가드 추가, baseStart/baseEnd 접근 전 범위 가드 추가.
- 동적 메모리/QVector → 함수 스코프 정적 배열 또는 max band width 기반 고정 배열로 치환. 정성 분석에서는 baseLineChartData 별도 저장 없이 루프 내 즉시 합산만 수행한다.
- 분모 0 → `(end - start) == 0` 등 모든 분모 계산에 가드 추가.
- QString::arg / qDebug → 전부 제거하며, 디버그 필요 시 단일 UART trace 매크로로 치환한다.

처리 시간 목표는 CALCULATING 1회 호출 기준 100 ms 미만이며, 1차 Trial 후 실측한다.

## 7. 이번 단계 제외 범위
다음 항목은 이번 Trial 범위에서 제외한다.
- 정량 분석(eQuant), applyCalibration, getSCoValue, 4PL/5PL
- ratioAB 후처리
- QC, InstrumentCheck, PD check
- BLE 결과 송신
- 결과 이력 저장(EEPROM/Flash)
- ERROR 화면 세분화(ERROR_LOT / ERROR_KIT_INVALID 등)
- 실제 Cassette 감지 인터페이스 (계속 버튼 Stub 유지)
- center 3-pass 분석 (1-pass로 시작)
- float/double 도입 (정수 기반만 사용)
- 3-pass center shift, max-score 채택 로직
- coef_x1000 / offset_score 적용 (LotParam 구조체 필드 예약만 수행, 실제 비교식 미적용)

## 8. 커밋 단위 합의
다음 6개 커밋 단위로 분할한다.

1. AnalysisEngine_Types 및 TestData 추가 (빌드만 통과)
2. AnalysisEngine_App 정성 분석 코어 구현 (정수 기반, single-pass)
3. CodeChip_TempLotData 추가 및 CodeChip_Interface_GetLotParam 신설
4. SequenceManager INCUBATION 상태 및 CALCULATING 1회 실행 정책 반영, s_last_result 보관 및 GetLastResult getter 추가
5. InputInterpreter INCUBATION_START 명령 신설 및 ScreenManager 결과 표시 연결
6. IAR 프로젝트(.ewp) 신규 파일 등록 및 E2E 흐름 빌드/검증

## 9. 후속 단계 검토 항목
- 실제 CodeChip 데이터 포맷 확정 후 CodeChip_Parser로 TempLotData 교체
- 광학 보드 도입 후 OpticalSampler로 TestData 교체
- center 3-pass 채택 여부 재검토 및 정확도 비교
- 정량 분석 도입 시 fixed-point(Q15/Q31) 또는 float 도입 재검토
- coef_x1000 / offset_score를 사용한 정량 보정식 도입 및 `int32_t` 오버플로 대비책(`int64_t` 임시 승격 포함) 검토
- CALCULATING 1회 실행 시간이 100 ms를 초과할 경우 baseline 나눗셈 회피 최적화 적용 여부 검토
- 결과 표시 후 Cassette 제거 안내, Result/Remove 교대 표시, Cassette 제거 감지 후 STANDBY 복귀 흐름을 다음 브랜치 범위로 설계
- ERROR 원인 세분화 및 RESET 입력원 정의
- BLE 결과 송신 경로 설계

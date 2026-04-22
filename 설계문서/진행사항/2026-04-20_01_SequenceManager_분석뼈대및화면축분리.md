# 2026-04-20_01_SequenceManager_분석뼈대및화면축분리.md

## 1. 작업 목적
현재 구조에서는 PowerManager가 제공하는 전원 상태만으로 ScreenManager가 화면을 직접 결정하고 있으며, InputInterpreter의 analysis 경로는 skeleton 상태로 남아 있다. 이 상태에서 코드칩 대기, 카세트 대기, 측정중, 계산중, 결과표시 같은 검사 단계 화면을 추가하기 시작하면 `POWERMANAGER_STATE_STANDBY` 아래에 검사 의미가 계속 누적되어 전원축과 검사축의 owner가 섞일 가능성이 크다.

이번 계획은 이런 문제를 ScreenManager 단독 과제로 다루기보다, SequenceManager 기반 분석 뼈대 도입 계획 안에 흡수해 정리하는 것을 목표로 한다. 현재 합의된 모듈 책임 기준은 아래와 같다.

- PowerManager = 전원 정책 및 전원 상태 전이만 담당
- InputInterpreter = 입력 원시 이벤트 해석 및 명령 변환/전달 담당
- ScreenManager = 최종 화면 장면 선택 및 렌더링 담당
- SequenceManager = 검사/분석 절차의 상태 전이만 담당
- AnalysisExecutor = UV/PD scan 실행 담당
- AnalysisEngine = raw data 기반 결과 계산 담당

따라서 이번 단계의 핵심 목적은 SequenceManager 최소 상태를 도입하고, InputInterpreter의 analysis 경로를 실제로 연결하며, ScreenManager가 전원 상태와 검사 상태를 조합해 최종 장면을 선택하도록 구조를 정리하는 데 있다.

## 2. 수행 내용

### 2.1 새 SequenceManager 모듈 추가
`USER/App/SequenceManager_App/` 경로에 `SequenceManager_App.h/.c`, `SequenceManager_Interface.h/.c` 파일을 새로 추가했다. 이 모듈은 검사 절차 전용 상태 owner로 동작하도록 만들었으며, 최소 상태 집합은 `IDLE`, `WAIT_CODECHIP`, `WAIT_CASSETTE`, `MEASURING`, `CALCULATING`, `RESULT_DISPLAY` 로 정의했다. 이번 작업의 목적은 검사 절차 상태를 ScreenManager나 PowerManager에 섞지 않고 별도 모듈에서 관리할 수 있는 기반을 만드는 데 있다.

### 2.2 SequenceManager 내부 상태머신 및 시작 요청 가드 추가
`SequenceManager_App.c` 에 명령 큐, 현재 상태 저장 변수, 상태 전이 처리 함수를 추가했다. `START_REQUEST`, `CODECHIP_READY`, `CASSETTE_READY`, `MEASUREMENT_DONE`, `CALCULATION_DONE`, `RESET` 명령에 따라 상태가 순차적으로 전이되도록 구현했고, 표시 가능 전원 상태가 아닐 때 들어온 시작 요청은 pending 처리 후 전원 복귀 뒤 반영하도록 구성했다. 이번 작업의 목적은 검사 절차 흐름을 한 곳에서 관리하고, SLEEP 상태 등에서 전원 복귀 전에 검사 상태가 먼저 진행되는 문제를 막는 데 있다.

### 2.3 SequenceManager 공개 인터페이스 추가
`SequenceManager_Interface.h/.c` 에 `Init`, `Run`, `SubmitCommand`, `GetState` 함수를 추가했다. 다른 App 모듈은 내부 구현을 직접 참조하지 않고 이 공개 API를 통해 SequenceManager에 접근하도록 정리했다. 이번 작업의 목적은 상태머신 내부 구현과 외부 호출 지점을 분리해 이후 수정 시 영향 범위를 줄이는 데 있다.

### 2.4 InputInterpreter의 analysis skeleton 제거 후 SequenceManager 연결
`InputInterpreter_App.c` 의 analysis 경로에서 기존 skeleton no-op 자리를 실제 SequenceManager 연결 코드로 교체했다. `INPUTINTERPRETER_CMD_ANALYSIS_START_REQUEST` 를 `SEQUENCEMANAGER_CMD_START_REQUEST` 로 변환해 SequenceManager로 전달하도록 수정했다. 이번 작업의 목적은 기존에 동작하지 않던 analysis target 경로를 실제 검사 절차 상태머신으로 연결하는 데 있다.

### 2.5 User_Main 초기화 및 실행 순서에 SequenceManager 반영
`USER/User_Main.c` 에 `SequenceManager_Interface_Init()` 와 `SequenceManager_Interface_Run()` 호출을 추가했다. 전체 흐름은 `InputInterpreter -> SequenceManager -> PowerManager -> ScreenManager` 순서가 되도록 정리했다. 이번 작업의 목적은 현재 주기에서 갱신된 검사 상태와 전원 상태를 ScreenManager가 함께 참고할 수 있도록 전체 호출 순서를 맞추는 데 있다.

### 2.6 ScreenManager 상태 enum에 검사 장면 추가
`ScreenManager_App.h` 에 `WAIT_CODECHIP`, `WAIT_CASSETTE`, `MEASURING`, `CALCULATING`, `RESULT_DISPLAY` 상태를 추가했다. 기존 `BOOT`, `STANDBY`, `SLEEP`, `BLANK`, `POWER_OFF_NOTICE` 상태는 유지했다. 이번 작업의 목적은 전원 장면과 검사 장면을 코드상에서 명시적으로 구분하고, 검사 절차에 대응하는 화면 상태를 표현할 수 있도록 하는 데 있다.

### 2.7 ScreenManager 장면 선택 로직을 전원축 + 검사축 조합 방식으로 변경
`ScreenManager_App.c` 에 PowerManager 상태와 SequenceManager 상태를 조합해 최종 화면을 결정하는 로직을 추가했다. `SLEEP`, `POWER_OFF_NOTICE`, `POWER_OFF` 는 검사 상태보다 우선 처리하고, 표시 가능 상태에서는 SequenceManager 상태에 따라 `INSERT CODECHIP`, `INSERT CASSETTE`, `MEASURING...`, `CALCULATING...`, `RESULT READY` 화면이 선택되도록 수정했다. 이번 작업의 목적은 ScreenManager가 검사 상태를 직접 소유하지 않고, 전원 상태와 검사 상태를 조합하는 최종 렌더 계층으로 동작하도록 만드는 데 있다.

### 2.8 IAR 프로젝트 파일에 SequenceManager 소스 등록
`EWARM/INCLIX_F1_LCD_Interface_20260223_v1_0.ewp` 에 `SequenceManager_App.c`, `SequenceManager_Interface.c` 를 새 그룹으로 추가했다. 새로 만든 SequenceManager 코드가 실제 빌드 대상에 포함되도록 프로젝트 파일도 함께 반영했다. 이번 작업의 목적은 소스 추가와 빌드 구성이 어긋나지 않도록 맞추는 데 있다.

## 3. 검증 결과

### 3.1 실기 테스트
- [x] 부팅 후 STANDBY에서 버튼 입력 시 `INSERT CODECHIP` 전환 확인
- [x] SLEEP에서 버튼 입력 시 `INCLIX STANDBY`를 잠깐 거친 뒤 `INSERT CODECHIP` 전환 확인 (슬로우모션)
- [x] `POWER_OFF_NOTICE` 진입 시 `TURNING OFF...` 표시 확인
- [x] `POWER_OFF` 진입 시 화면 BLANK 확인
- [x] BAT LOW 상태에서 display scene에만 `BAT LOW` overlay가 표시되는지 확인

### 3.2 코드 리뷰 확인
- [x] `STANDBY` 의미가 "표시 가능 전원 상태"로 코드와 문서에서 일관적인지 확인한다. — `SequenceManager_IsDisplayCapableState()` 가 `POWERMANAGER_STATE_STANDBY` 일 때만 `1` 반환
- [x] SequenceManager가 검사 절차 상태 전이만 담당하고, scan 실행/결과 계산/LCD 렌더링을 맡지 않는지 확인한다. — `SequenceManager_App.c` 에 `ST7735S`, `AnalysisExecutor`, `AnalysisEngine` 호출 없음
- [x] InputInterpreter의 analysis 경로가 skeleton no-op이 아니라 실제 SequenceManager submit으로 연결되는지 확인한다. — `InputInterpreter_DispatchToAnalysis()` 가 `SEQUENCEMANAGER_CMD_START_REQUEST` 로 변환해 submit 호출
- [x] ScreenManager가 전원 상태(`SLEEP`, `POWER_OFF_NOTICE`, `POWER_OFF`)를 검사 상태보다 우선 처리하는지 확인한다. — `ScreenManager_ResolveState()` 에서 전원 상태를 seq_state 보다 먼저 처리
- [x] 기존 BOOT hold, redraw 억제, sleep timeout, power-off notice timeout 정책에 회귀가 없는지 확인한다. — `SCREEN_BOOT_HOLD_MS`, `s_need_redraw`, `idle_timeout_ms`(600s), `sleep_timeout_ms`(300s), `power_off_notice_timeout_ms`(3s) 모두 유지

## 4. 잔여 이슈/TODO
- SequenceManager 상태별 실제 입력 이벤트 집합과 세부 전이 조건을 후속 단계에서 더 구체화해야 한다.
- `User_Main` 에서 SequenceManager와 PowerManager의 호출 순서를 activity 갱신 정책과 함께 다시 검토해야 한다.
- 코드칩 감지, 카세트 감지, 측정 완료, 계산 완료 이벤트를 어떤 생산자가 보낼지 아직 정해지지 않았다.
- AnalysisExecutor와 AnalysisEngine을 SequenceManager 내부에서 직접 호출할지, 상위 orchestration 지점에서 연결할지 후속 결정이 필요하다.
- 분석 시작 요청을 PowerManager의 표시 가능 전원 상태와 어떻게 동기화할지 구체 규칙을 확정해야 한다.
- `WAIT_REMOVE` 생산자와 `RESULT_DISPLAY` 이후 복귀 조건을 후속 단계에서 구체화해야 한다.
- 결과 화면은 우선 단일 `RESULT_DISPLAY` 장면으로 시작하고, 상세 결과 페이지나 추가 UI 흐름은 후속 단계로 분리하는 편이 안전하다.
# 2026-04-29_01_AnalysisPrepare_StubFlow_합의계획

## 1. 문서 목적
본 문서는 분석 준비 단계 Stub 검증 브랜치에서 현재까지 합의된 설계 방향을 정리한다.
이번 범위는 구현 착수 전 설계 확정이며, 실제 구현 상세와 후속 기능은 제외한다.

## 2. 배경
현재 제품 시나리오는 다음 흐름을 목표로 한다.

STANDBY -> INSERT CODECHIP -> Lot 검증 -> INSERT CASSETTE -> PRESS START -> Incubation

다만 현재 단계 제약은 다음과 같다.
- CodeChip 실물은 있으나 내부 데이터 포맷은 미정
- Cassette 실물 없음
- 따라서 실제 Lot 파싱과 실제 Cassette 감지는 이번 단계에서 구현하지 않음

## 3. 브랜치 방향
브랜치명 후보: feature/analysis_prepare_stub_flow

이번 브랜치의 종료 목표 상태는 READY_TO_INCUBATE(PRESS START)이며, Incubation 진입은 다음 단계에서 다룬다.

## 4. 합의된 핵심 설계
### 4.1 상태 확장
SequenceManager 상태에 다음 항목을 추가한다.
- VALIDATE_LOT
- READY_TO_INCUBATE
- ERROR

### 4.2 명령 확장
SequenceManager 명령에 다음 항목을 추가한다.
- CODECHIP_INSERTED
- LOT_VALID
- LOT_INVALID
- LOT_READ_FAIL
- CASSETTE_INSERTED

기존 명령명 CODECHIP_READY는 CODECHIP_INSERTED로 변경한다.

변경 대상:
- INPUTINTERPRETER_CMD_CODECHIP_READY -> INPUTINTERPRETER_CMD_CODECHIP_INSERTED
- SEQUENCEMANAGER_CMD_CODECHIP_READY -> SEQUENCEMANAGER_CMD_CODECHIP_INSERTED

### 4.3 VALIDATE_LOT 처리 정책
- LotValidator Stub 결과 처리는 HandleCommand 내부 즉시 호출이 아니라 SequenceManager_App_Run() 실행 흐름에서 수행한다.
- VALIDATE_LOT 반복 평가 방지를 위해 1회 처리 플래그를 둔다.
- 1회 처리 플래그는 VALIDATE_LOT 상태 진입 시점에 리셋한다.

예시 정책:
- next_state가 VALIDATE_LOT일 때 s_lot_validation_done를 0으로 초기화
- 동일 상태 체류 중에는 재검증하지 않음

### 4.4 VALIDATE_LOT timeout 정책
- 이번 단계에서는 timeout 동작을 구현하지 않는다.
- 이유: LotValidator가 Stub이며 즉시 결과를 반환함
- 향후 실제 read/parse 도입을 대비해 s_state_enter_tick 기반 timeout TODO 주석만 남긴다.

### 4.5 ERROR 복귀 정책
- ERROR + RESET -> IDLE
- any state + RESET -> IDLE
- 버튼 클릭으로 ERROR 자동 복귀는 이번 단계에서 허용하지 않음
- RESET 처리 로직은 유지하되, RESET 입력원 추가 연결은 보류

### 4.6 READY_TO_INCUBATE 정책
- WAIT_CASSETTE + CASSETTE_INSERTED -> READY_TO_INCUBATE
- READY_TO_INCUBATE에서 분석 다음 전이는 만들지 않음
- 버튼 클릭에 대한 Power USER_ACTIVITY는 기존처럼 유지 가능
- START_REQUEST가 들어와도 READY_TO_INCUBATE 상태를 유지한다.

### 4.7 Cassette Stub 정책
- Cassette 실물이 없으므로 Cassette_Drv, CassetteMonitor_App는 이번 단계에서 생성하지 않음
- WAIT_CASSETTE 상태에서 버튼 클릭을 CASSETTE_INSERTED Stub 이벤트로 간주
- 해당 코드는 TEMP STUB 주석으로 명확히 표기
- 이후 CassetteMonitor_Interface 도입 시 해당 Stub 분기를 대체

### 4.8 ScreenManager 화면 정책
- VALIDATE_LOT -> CHECKING LOT
- READY_TO_INCUBATE -> PRESS START
- ERROR -> LOT ERROR

이번 단계에서는 ERROR 세부 원인(LOT_INVALID, LOT_READ_FAIL)별 화면 분리는 하지 않는다.
세부 분리는 이후 ErrorManager 도입 시 검토한다.

## 5. 목표 검증 흐름
STANDBY
-> 버튼 클릭
-> INSERT CODECHIP
-> CodeChip 삽입
-> CHECKING LOT
-> LotValidator Stub VALID
-> INSERT CASSETTE
-> 버튼 클릭으로 Cassette inserted 가정
-> PRESS START

## 6. 이번 브랜치 포함 범위
- CODECHIP_READY -> CODECHIP_INSERTED 이름 변경
- SequenceManager 상태/명령 확장
- CodeChip_LotValidator Stub 추가
- CodeChip_LotValidator.c 신규 추가에 따른 IAR 프로젝트(.ewp) 반영
- VALIDATE_LOT 상태에서 Stub 결과 1회 처리
- WAIT_CASSETTE 상태 버튼 기반 Cassette Stub 처리
- ScreenManager 신규 장면 반영
- 본 문서 기준 설계 근거 기록

## 7. 이번 브랜치 제외 범위
- 실제 CodeChip 데이터 파싱
- 실제 Lot 데이터 구조 확정
- 실제 Cassette GPIO 감지
- Cassette_Drv
- CassetteMonitor_App
- Incubation 구현
- UV LED 제어
- PD 샘플링
- TRF 계산
- 결과 저장
- BLE 통신
- PC 설정 모드

## 8. 구조 적합성 결론
본 방향은 현재 계층 구조를 유지한다.
- main -> User_Main -> App -> Interface/Driver -> User_HAL

또한 역할 분리가 유지된다.
- SequenceManager: 검사 절차 상태 전이 담당
- InputInterpreter: 버튼/CodeChip 이벤트를 의미 명령으로 변환
- ScreenManager: 전원 우선순위 + 검사 상태 기반 화면 결정

## 9. 구현 시 고정 정책
- VALIDATE_LOT 1회 처리 플래그는 상태 진입 시점에 리셋한다.
- READY_TO_INCUBATE에서 START_REQUEST는 이번 브랜치에서 분석 전이로 처리하지 않는다.
- ERROR 화면 문구는 이번 브랜치에서 LOT ERROR 단일 문구로 유지한다.
- RESET 처리 로직은 유지하되, RESET 입력원은 이번 브랜치에서 새로 추가하지 않는다.

위 정책에 따라 이번 브랜치에서는 ERROR 복귀 UX를 완성하지 않고, 복귀 구조만 준비한다.
실제 RESET 입력원과 ERROR 복귀 사용자 정책은 후속 브랜치에서 정의한다.

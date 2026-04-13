# INCLIX F-1 아키텍처 리팩토링 1차 설계안 (IDDD 모방 기반)

## 1. 문서 목적
본 문서는 INCLIX F-1 펌웨어를 리팩토링할 때, IDDD의 검증된 구조 중 핵심만 선택적으로 모방하기 위한 1차 설계 기준을 정의한다.

핵심 방향은 다음과 같다.
1. 입력을 즉시 처리하지 않고 이벤트와 명령으로 분리한다.
2. 입력 해석 계층을 두어 원시 이벤트를 의미 있는 명령으로 변환한다.
3. 분석 흐름은 별도 SequenceManager 스타일 상태머신으로 운영한다.
4. IDDD의 단일 과책임 허브 구조는 도입하지 않는다.

## 2. 설계 원칙
1. Driver는 HW 접근만 담당한다.
2. App은 정책과 상태 전이 등 로직만 담당한다.
3. Interface는 외부 공개 API만 담당한다.
4. InputInterpreter는 해석만 수행하며 HW 제어를 하지 않는다.
5. AnalysisSequenceManager는 분석 흐름만 담당하며 전원 정책을 직접 다루지 않는다.
6. PowerManager는 전원 정책만 담당한다.
7. ScreenManager는 화면 선택과 전환 정책만 담당한다.
8. User_Main은 엔트리 및 스케줄링만 담당한다.

## 3. 목표 구조
구조 흐름은 아래와 같이 정리한다.

입력 이벤트 생산자
→ InputInterpreter
→ AnalysisSequenceManager / PowerManager / ScreenManager
→ 각 도메인 실행 모듈

입력 이벤트 생산자에는 Button, Battery를 우선 적용하고 이후 USB, Cassette, CodeChip을 확장한다.

## 4. 현재 코드 기준 주요 충돌 지점
1. User_Main이 BatteryDisplay를 직접 호출하고 있어 ScreenManager 중심 구조와 충돌한다.
2. BatteryDisplay가 배터리 이벤트를 직접 소비하고 있어 InputInterpreter 단일 해석 관문과 충돌한다.
3. PowerControl_App에 long-press 의미 해석과 USB 조건 정책이 포함되어 있어 PowerManager 단일 정책 원칙과 충돌한다.
4. Button 큐 overflow 반환값이 호출부에서 무시되어 이벤트 유실 진단이 어렵다.

## 5. 1차 구현 범위
1. 기존 Button, Battery 이벤트 구조는 유지한다.
2. InputInterpreter 계층을 신규 추가한다.
3. AnalysisSequenceManager 스켈레톤을 추가한다.
4. PowerManager 스켈레톤을 추가한다.
5. ScreenManager 스켈레톤을 추가한다.
6. User_Main 호출 구조를 엔트리 및 스케줄러 형태로 개편한다.
7. BatteryDisplay_App는 전체 화면 관리가 아닌 부분 렌더링 부품으로 축소한다.
8. PowerControl_App는 정책 판단이 아닌 HW power hold 제어 부품으로 축소한다.

## 6. 모듈별 최소 계약 초안

### 6.1 공통 명령 타입
1. InputCommand 식별자
- NONE
- BTN_PAUSE_SHORT
- BTN_PAUSE_LONG
- BAT_LEVEL_CHANGED
- BAT_CRITICAL_DETECTED

2. InputCommand 필드
- id
- param0
- timestamp_ms

3. 공통 반환값
- OK
- EMPTY
- FULL
- INVALID

### 6.2 InputInterpreter
1. 입력 제출 API
- Button 원시 이벤트 제출
- Battery 원시 이벤트 제출
2. 실행 API
- 내부 큐 처리
3. 출력 API
- 의미 명령 조회

### 6.3 AnalysisSequenceManager
1. 상태
- IDLE
- READY
- PRECHECK
- RUNNING
- COMPLETE
- ERROR
2. API
- Init
- Run
- SubmitCommand
- GetState

### 6.4 PowerManager
1. 상태
- ON
- PREPARE_OFF
- OFF
- USB_LOCK
2. API
- Init
- Run
- SubmitCommand
- GetState

### 6.5 ScreenManager
1. 상태
- BOOT
- IDLE
- BATTERY
- ANALYSIS_PROGRESS
- RESULT
- POWER_OFF_HINT
2. API
- Init
- Run
- SubmitCommand
- GetState
3. 정책
- BatteryDisplay 호출 여부는 Screen 상태로만 결정

## 7. 기존 수정 대상 파일
1. USER/User_Main.c
2. USER/App/BatteryDisplay_App/BatteryDisplay_App.h
3. USER/App/BatteryDisplay_App/BatteryDisplay_App.c
4. USER/App/PowerControl_App/PowerControl_App.h
5. USER/App/PowerControl_App/PowerControl_App.c

## 8. 충돌 표시 주석 삽입 원칙
1. User_Main 직접 화면 호출 위치에 ScreenManager 경유 전환 예정 주석 삽입
2. BatteryDisplay 직접 이벤트 소비 위치에 InputInterpreter 이관 예정 주석 삽입
3. PowerControl 정책 판단 위치에 PowerManager 이관 예정 주석 삽입
4. Button queue push 실패 무시 위치에 유실 진단 보강 필요 주석 삽입

## 9. 검증 계획
1. 빌드 검증
- EWARM 전체 빌드 성공
- 신규 모듈 링크 확인

2. 이벤트 경로 검증
- Button, Battery 원시 이벤트가 의미 명령으로 변환되는지 확인

3. 상태 전이 검증
- Analysis 상태 전이
- Power 상태 전이
- Screen 상태 전이

4. 안정성 검증
- queue overflow 처리 정책 확인
- HAL 반환값 점검 경로 확인
- timeout 상수화 확인
- ISR와 main 공유 상태 보호 규칙 확인

## 10. 제외 범위
1. USB, Cassette, CodeChip의 상세 도메인 로직 구현
2. 저장, 통신 등 고급 연동 로직
3. 고급 복구 시나리오 완성
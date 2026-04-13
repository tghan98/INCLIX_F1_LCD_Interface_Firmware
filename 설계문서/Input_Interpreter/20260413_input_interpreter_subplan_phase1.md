# INCLIX F-1 InputInterpreter 1차 서브계획 최종본

## 1. 목적
본 문서는 INCLIX F-1 구조 리팩토링의 단계적 실행을 위해, InputInterpreter 계층을 우선 도입하는 1차 서브계획을 정의한다.

이번 단계의 목표는 다음과 같다.
1. 기존 Button/Battery 이벤트 생산 구조를 유지한다.
2. 원시 이벤트를 직접 처리하지 않고 의미 명령으로 변환하는 얇은 해석 계층을 추가한다.
3. InputInterpreter가 PowerManager와 AnalysisSequenceManager로 명령을 전달할 수 있는 구조를 만든다.
4. User_Main에 InputInterpreter Init/Run 호출 구조를 연결한다.

## 2. 범위
이번 단계에서 포함한다.
1. InputInterpreter_App / InputInterpreter_Interface 생성
2. Button_Interface_GetEvent()로 버튼 이벤트 소비
3. BatteryMonitor_Interface_GetEvent()로 배터리 이벤트 소비
4. 원시 이벤트를 Power/Analysis 명령으로 변환하는 규칙 추가
5. PowerManager 명령 전달 연결
6. AnalysisSequenceManager 전달 연결점(TODO skeleton) 추가
7. User_Main Init/Run 호출 추가

이번 단계에서 제외한다.
1. AnalysisSequenceManager 실제 구현
2. ScreenManager 구현
3. InputInterpreter의 GPIO/HW 직접 제어
4. InputInterpreter의 LCD 직접 제어
5. InputInterpreter의 분석 실행 함수 직접 호출
6. Button_App/BatteryMonitor_App 구조 변경
7. User_Main에 해석 정책 if문 추가

## 3. 설계 원칙
1. Driver는 HW 접근만 담당한다.
2. App은 로직 및 상태 전이만 담당한다.
3. Interface는 외부 공개 API만 담당한다.
4. InputInterpreter는 해석 및 라우팅만 담당한다.
5. InputInterpreter는 도메인 실행 모듈을 직접 실행하지 않는다.

## 4. 파일 계획

### 4.1 신규 파일
1. USER/App/InputInterpreter_App/InputInterpreter_App.h
2. USER/App/InputInterpreter_App/InputInterpreter_App.c
3. USER/App/InputInterpreter_App/InputInterpreter_Interface.h
4. USER/App/InputInterpreter_App/InputInterpreter_Interface.c

### 4.2 수정 파일
1. USER/User_Main.c

## 5. 타입 및 계약 최소 정의

### 5.1 타깃 enum
1. INPUTINTERPRETER_TARGET_NONE
2. INPUTINTERPRETER_TARGET_POWER
3. INPUTINTERPRETER_TARGET_ANALYSIS

### 5.2 명령 enum
1. INPUTINTERPRETER_CMD_NONE
2. INPUTINTERPRETER_CMD_USER_ACTIVITY
3. INPUTINTERPRETER_CMD_ANALYSIS_START_REQUEST
4. INPUTINTERPRETER_CMD_POWER_OFF_REQUEST
5. INPUTINTERPRETER_CMD_POWER_BATTERY_LOW
6. INPUTINTERPRETER_CMD_POWER_BATTERY_CRITICAL

### 5.3 해석 패킷 struct
1. target
2. cmd
3. param0
4. timestamp_ms

## 6. API 최소 구성

### 6.1 InputInterpreter_App
1. InputInterpreter_App_Init
2. InputInterpreter_App_Run
3. InputInterpreter_App_GetLastTranslated

### 6.2 InputInterpreter_Interface
1. InputInterpreter_Interface_Init
2. InputInterpreter_Interface_Run

## 7. 변환 규칙 기준
1. 버튼 short press/click 이벤트는 사용자 활동 또는 분석 시작 요청 계열 명령으로 변환한다.
2. 버튼 long press는 현재 드라이버 이벤트 부재 시 TODO skeleton으로 유지한다.
3. battery low는 PowerManager battery low 명령으로 변환한다.
4. battery critical은 PowerManager battery critical 명령으로 변환한다.
5. Analysis 전달은 실제 구현 전까지 연결점 함수(TODO)로 유지한다.

## 8. User_Main 적용 기준
1. Init: InputInterpreter_Interface_Init 호출 추가
2. Run: Button/Battery 이벤트 생산 이후 InputInterpreter_Interface_Run 호출 추가
3. User_Main에는 해석 정책 분기 로직을 추가하지 않는다.
4. 기존 Button/Battery 생산 구조는 그대로 유지한다.

## 9. 검증 기준
1. 빌드 성공: 신규 파일 추가 후 컴파일/링크 오류 없음
2. 이벤트 소비: InputInterpreter가 Button/Battery 큐를 정상 소비함
3. 변환 구조: 원시 이벤트에서 Power/Analysis 명령으로 변환 경로 존재
4. 전달 구조: PowerManager 명령 전달 연결 동작 확인
5. 연결점 구조: Analysis 전달 연결점(TODO) 존재 확인
6. 책임 분리: InputInterpreter에서 GPIO/LCD/분석 직접 실행 호출 없음

## 10. 산출물
1. InputInterpreter 관련 4개 파일 생성
2. User_Main 호출 구조 반영
3. 본 서브계획 문서 저장 완료

# INCLIX F-1 PowerManager 1차 서브계획 최종본

## 1. 목적
본 문서는 INCLIX F-1 구조 리팩토링의 단계적 실행을 위해, Power 영역을 우선 적용하는 1차 서브계획을 정의한다.

이번 단계의 목표는 다음과 같다.
1. PowerManager App/Interface 뼈대를 생성한다.
2. 전원 상태머신 skeleton을 도입한다.
3. 상태 enum, 모드 enum, 명령 enum, context struct를 정의한다.
4. User_Main에서 Init/Run 호출 구조를 연결한다.

## 2. 범위
이번 단계에서 포함한다.
1. PowerManager_App / PowerManager_Interface 생성
2. BOOT -> STANDBY -> SLEEP -> POWER_OFF_PENDING -> POWER_OFF 상태 흐름 skeleton 추가
3. charging/discharging 모드 enum 및 상태 보관 구조 추가
4. idle/sleep/power off pending timeout 틀 추가
5. battery low/critical 정책 수신 준비용 명령 수신 API 추가
6. User_Main Init/Run 호출 추가

이번 단계에서 제외한다.
1. InputInterpreter 구현
2. AnalysisSequenceManager 구현
3. ScreenManager 구현
4. PowerManager의 Button GPIO 직접 읽기
5. PowerManager의 LCD 직접 출력
6. User_Main 내 전원 정책 조건문 확장

## 3. 설계 원칙
1. Driver는 HW 접근만 담당한다.
2. App은 로직 및 상태 전이만 담당한다.
3. Interface는 외부 공개 API만 담당한다.
4. PowerManager는 전원 정책만 담당한다.
5. PowerControl_App와 동시 정책 실행을 금지한다.

## 4. 파일 계획

### 4.1 신규 파일
1. USER/App/PowerManager_App/PowerManager_App.h
2. USER/App/PowerManager_App/PowerManager_App.c
3. USER/App/PowerManager_App/PowerManager_Interface.h
4. USER/App/PowerManager_App/PowerManager_Interface.c

### 4.2 수정 파일
1. USER/User_Main.c

## 5. 타입 및 계약 최소 정의

### 5.1 상태 enum
1. POWERMANAGER_STATE_BOOT
2. POWERMANAGER_STATE_STANDBY
3. POWERMANAGER_STATE_SLEEP
4. POWERMANAGER_STATE_POWER_OFF_PENDING
5. POWERMANAGER_STATE_POWER_OFF

### 5.2 모드 enum
1. POWERMANAGER_MODE_UNKNOWN
2. POWERMANAGER_MODE_CHARGING
3. POWERMANAGER_MODE_DISCHARGING

### 5.3 명령 enum
1. POWERMANAGER_CMD_NONE
2. POWERMANAGER_CMD_ACTIVITY
3. POWERMANAGER_CMD_FORCE_SLEEP
4. POWERMANAGER_CMD_WAKEUP
5. POWERMANAGER_CMD_BATTERY_LOW
6. POWERMANAGER_CMD_BATTERY_CRITICAL
7. POWERMANAGER_CMD_CHARGER_ATTACHED
8. POWERMANAGER_CMD_CHARGER_DETACHED

### 5.4 context struct
1. state
2. mode
3. state_enter_tick
4. last_activity_tick
5. idle_timeout_ms
6. sleep_timeout_ms
7. power_off_pending_timeout_ms
8. battery_low_latched
9. battery_critical_latched

## 6. API 최소 구성

### 6.1 PowerManager_App
1. PowerManager_App_Init
2. PowerManager_App_Run
3. PowerManager_App_SubmitCommand
4. PowerManager_App_GetState
5. PowerManager_App_GetMode
6. PowerManager_App_GetContext

### 6.2 PowerManager_Interface
1. PowerManager_Interface_Init
2. PowerManager_Interface_Run
3. PowerManager_Interface_SubmitCommand
4. PowerManager_Interface_GetState
5. PowerManager_Interface_GetMode
6. PowerManager_Interface_GetContext

## 7. 상태머신 skeleton 기준
1. 초기화 후 BOOT 상태로 진입한다.
2. BOOT는 최소 초기 전이로 STANDBY로 넘어간다.
3. STANDBY는 idle timeout 경과 시 SLEEP으로 전이한다.
4. SLEEP은 sleep timeout 경과 시 POWER_OFF_PENDING으로 전이한다.
5. POWER_OFF_PENDING은 pending timeout 경과 시 POWER_OFF로 전이한다.
6. POWER_OFF의 실제 HW 차단 동작은 후속 단계에서 연결한다.

## 8. User_Main 적용 기준
1. Init: PowerManager_Interface_Init 호출 추가
2. Run: PowerManager_Interface_Run 호출 추가
3. User_Main은 스케줄 호출만 수행하고 정책 분기 로직을 추가하지 않는다.
4. 기존 Button/Battery 구조는 유지한다.

## 9. PowerControl_App 충돌 회피 원칙
1. 1차 단계에서는 PowerControl_App 호출을 계속 비활성 상태로 유지한다.
2. PowerManager 정책과 PowerControl_App 정책의 동시 실행을 금지한다.
3. 2차 단계에서 PowerControl_App를 HW 제어 전용으로 축소할지, 최종 삭제할지 결정한다.

## 10. 검증 기준
1. 빌드 성공: 신규 파일 추가 후 컴파일/링크 에러 없음
2. 호출 성공: User_Main Init/Run에서 PowerManager Interface 호출 가능
3. 상태 확인: BOOT -> STANDBY 기본 전이 동작 확인
4. 책임 분리 확인: PowerManager에서 GPIO 직접 읽기/LCD 출력 없음
5. 기존 구조 보호: Button/Battery 이벤트 경로가 기존과 동일하게 유지됨

## 11. 산출물
1. PowerManager 관련 4개 파일 생성
2. User_Main 호출 구조 반영
3. 본 서브계획 문서 저장 완료

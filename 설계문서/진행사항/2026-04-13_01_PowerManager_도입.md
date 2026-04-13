# 2026-04-13 PowerManager도입 phase01

## 1. 작업 목적
기존 전원 제어 로직의 책임을 분리하기 위해 PowerManager를 도입하고, 전원 정책을 상태머신 기반으로 운영할 수 있는 최소 구조를 마련한다. 또한 User_Main을 정책 실행자가 아닌 스케줄러로 유지해 이후 InputInterpreter/Analysis 연동 시 구조 충돌을 줄인다.

## 2. 수행내용

### 2.1 PowerManager App/Interface 도입
PowerManager를 App과 Interface 두 계층으로 나눠 구현했다.

- **Interface**: 외부 모듈(User_Main, InputInterpreter 등)이 PowerManager를 사용할 수 있는 창구 역할만 담당한다. 제공하는 함수는 `Init`, `Run`, `SubmitCommand`, `GetState` 4종류이며, 내부 구현을 몰라도 이 함수만 호출하면 된다. 내부적으로는 아무 로직 없이 App 함수로 그대로 전달하기만 한다.
- **App**: 실제 전원 정책 로직을 담당한다. 명령 큐에서 명령을 꺼내 처리하고, 상태머신을 실행해 전원 상태를 전이시킨다.

이 구조 덕분에 외부 모듈은 App 내부를 몰라도 되고, App 구현이 바뀌어도 Interface 함수 이름이 유지되는 한 외부 코드를 수정할 필요가 없다.

### 2.2 상태/모드/명령/컨텍스트 구조 정의
PowerManager는 아래 4가지만 보고 동작한다.

- 상태: 지금 전원이 어느 단계인지
	BOOT(시작), STANDBY(대기), SLEEP(절전), POWER_OFF_PENDING(꺼질 준비), POWER_OFF(꺼짐)
- 모드: 배터리가 충전 중인지 방전 중인지
	CHARGING(충전), DISCHARGING(방전), UNKNOWN(아직 모름)
- 명령: 상태를 바꾸게 만드는 사건
	USER_ACTIVITY(사용자 동작), BATTERY_LOW(배터리 낮음), BATTERY_CRITICAL(배터리 위험), FORCE_SLEEP(강제 절전), WAKEUP(절전 해제)
- 컨텍스트: 판단할 때 참고하는 기록
	timeout(얼마나 시간이 지났는지), latch(저전압/위험전압이 발생했는지 기록)

한 줄로 말하면, 상태는 현재 위치, 모드는 배터리 방향, 명령은 변화 신호, 컨텍스트는 판단용 메모다.

### 2.3 User_Main 스케줄 연결 및 충돌 회피
User_Main의 Init/Run에 PowerManager_Interface 호출을 추가해 메인 루프에서 상태머신이 주기 실행되도록 연결했다. 동시에 기존 PowerControl_App는 호출 비활성 상태를 유지해 정책 이중 실행 충돌을 방지했다.

## 3. 검증 결과
빌드가 정상 완료되었고, PowerManager 관련 호출 경로가 User_Main에서 정상 연결됨을 확인했다. 전원 상태머신의 기본 전이(BOOT 진입 후 STANDBY 이동)가 코드 경로상 유효함을 확인했다.

## 4. 잔여 이슈 / TODO
InputInterpreter 연동 전 단계라 명령 유입은 제한적이며, battery 이벤트 기반 정책 검증은 후속 연동이 필요하다. PowerControl_App는 아직 코드가 남아 있으므로 HW 제어 전용으로 축소할지 최종 삭제할지 후속 결정이 필요하다.

## 5. 다음 단계
InputInterpreter와 PowerManager를 실제로 연동해 battery low/critical 전달을 검증한다. ScreenManager와 상태 연계를 추가해 전원 상태 변화에 따른 화면 정책 반영 경로를 정리한다. 이후 PowerControl_App 정리 방향을 확정해 전원 도메인 책임 경계를 마무리한다.

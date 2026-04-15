# 2026-04-15_01_ScreenManager_이름정리및상태분리.md

## 1. 작업 목적
ScreenManager와 PowerManager, InputInterpreter 사이에서 같은 의미를 다른 이름으로 쓰거나 서로 다른 의미를 같은 화면 상태로 재사용하는 문제를 정리한다. 이번 단계에서는 READY/IDLE/STANDBY 용어를 STANDBY 기준으로 통일하고, POWER_OFF가 SLEEP 화면 상태를 재사용하지 않도록 화면 표현 상태를 분리하되, timeout 정책과 상태 전이 로직은 변경하지 않는다.

## 2. 수행내용

### 2.1 수정 범위와 제외 범위 확정
이번 작업은 이름 정리와 화면 상태 분리에 집중하되, 혼선을 줄이기 위해 sleep 요청 계열 명령 이름도 함께 정리한다. ScreenManager의 상태 enum, 화면 명령 enum, 표시 문자열, PowerManager 상태와 ScreenManager 상태의 매핑 로직, InputInterpreter와 PowerManager 사이의 sleep 요청 명령 이름을 대상으로 삼는다. 반면 timeout 값, 상태 전이 조건, AnalysisSequenceManager 구현, long-press 기능 추가, 배터리 정책 변경은 이번 단계에서 제외한다.

### 2.2 ScreenManager 상태명과 화면 문구 정리 방향 확정
현재 ScreenManager는 내부 화면 상태에 `IDLE`을 사용하면서 실제 표시 문자열은 "INCLIX READY"를 출력하고 있고, 상위 전원 정책에서는 `STANDBY`라는 이름을 사용하고 있다. 같은 의미를 서로 다른 이름으로 유지하면 추후 문서와 코드 해석이 계속 어긋나므로, ScreenManager 쪽 이름을 `STANDBY` 기준으로 맞추기로 했다. 이에 따라 `SCREENMANAGER_STATE_IDLE`은 `SCREENMANAGER_STATE_STANDBY`로, `SCREENMANAGER_CMD_SHOW_IDLE`은 `SCREENMANAGER_CMD_SHOW_STANDBY`로 변경하고, 표시 문자열도 "INCLIX STANDBY" 또는 필요 시 "STANDBY"로 정리하는 방향을 잡았다.

### 2.3 POWER_OFF 화면 상태 분리 방향 확정
현재 구현에서는 `POWERMANAGER_STATE_POWER_OFF`가 ScreenManager에서 `SCREENMANAGER_STATE_SLEEP`로 매핑되어 화면 소거 상태를 재사용하고 있다. 화면상 결과는 같더라도 SLEEP과 POWER_OFF는 정책 의미가 다르므로, 이번 단계에서 ScreenManager 내부에 전용 blank 화면 상태를 추가해 의미를 분리하기로 했다. 이 상태명은 `SCREENMANAGER_STATE_POWER_OFF`보다 `SCREENMANAGER_STATE_BLANK`를 우선 검토한다. 이유는 ScreenManager가 전원 정책 상태명을 그대로 복제하기보다, 화면 표현 관점에서 "빈 화면"을 나타내는 독립 상태를 갖는 편이 더 느슨하게 결합되고 이후 확장에도 유리하기 때문이다.

### 2.4 명령 이름 정리 방향 확정
현재 구현에서는 `INPUTINTERPRETER_CMD_POWER_OFF_REQUEST`가 `POWERMANAGER_CMD_FORCE_SLEEP`로 번역되며, 실제 동작은 전원 차단이 아니라 sleep 진입 요청이다. 이름과 실제 동작이 어긋난 상태를 그대로 두면 문서와 코드 해석이 계속 헷갈리므로, 이번 단계에서 두 이름을 함께 `SLEEP_REQUEST` 계열로 맞추기로 했다. 다만 이 rename은 현재 동작을 정확히 드러내기 위한 정리이며, 장래의 실제 전원 종료 요청 개념까지 흡수하는 것은 아니다. 즉, 이번 단계에서는 `INPUTINTERPRETER_CMD_SLEEP_REQUEST`와 `POWERMANAGER_CMD_SLEEP_REQUEST`를 도입하되, 향후 실제 power off 요청이 필요해지면 별도 명령을 새로 추가하는 전제로 정리한다.

### 2.5 실제 수정 대상 파일 정리
실제 구현 시 주요 수정 대상은 `USER/App/ScreenManager_App/ScreenManager_App.h`, `USER/App/ScreenManager_App/ScreenManager_App.c`, `USER/App/InputInterpreter_App/InputInterpreter_App.h`, `USER/App/InputInterpreter_App/InputInterpreter_App.c`, `USER/App/PowerManager_App/PowerManager_App.h`, `USER/App/PowerManager_App/PowerManager_App.c`다. 이 중 ScreenManager 두 파일은 이번 단계의 핵심 수정 대상이고, InputInterpreter와 PowerManager는 sleep 요청 명령 rename을 반영하는 최소 범위 수정 대상으로 포함한다. 구현 후에는 변경 전/후 이름 매핑표를 함께 정리해 문서와 코드가 같은 용어를 사용하도록 맞출 예정이다.

## 3. 검증 결과
- [x] 실제 영향 범위 파일 목록 확인 완료
- [x] READY / IDLE / STANDBY 용어 충돌 지점 확인 완료
- [x] POWER_OFF가 SLEEP 화면 상태를 재사용하는 구조 확인 완료
- [x] 명령 이름을 `SLEEP_REQUEST` 계열로 함께 정리하는 방향 합의 완료
- [x] ScreenManager enum 및 문자열 rename 적용
- [x] POWER_OFF 전용 blank 화면 상태 추가 및 매핑 분리
- [x] InputInterpreter / PowerManager sleep 요청 명령 rename 적용
- [x] 빌드 통과
- [x] 실기: 초기 `INCLIX STANDBY` 표시 확인
- [x] 실기: 무입력 약 10분 후 SLEEP 진입에 따라 텍스트 소거 확인
- [x] 실기: SLEEP 상태에서 버튼 입력 시 `INCLIX STANDBY` 복귀 확인
- [x] 실기: SLEEP 상태 약 5분 후 `TURNING OFF...` 약 3초 출력 확인
- [x] 실기: `TURNING OFF...` 이후 화면 꺼짐 확인
- [x] 실기: 충전 상태에서 약 40분 관찰 동안 SLEEP 진입 후에도 `TURNING OFF...` 미발생 확인

## 4. 잔여 이슈/TODO
- blank 화면 상태명은 `SCREENMANAGER_STATE_BLANK`를 우선 검토하지만, 팀 내 용어 선호에 따라 `SCREENMANAGER_STATE_POWER_OFF`를 선택할 가능성도 남아 있다.
- 표시 문자열은 "INCLIX STANDBY"를 기본안으로 두되, 화면 폭이나 가독성 문제에 따라 "STANDBY" 단독 표기로 조정할 여지가 있다.
- `INPUTINTERPRETER_CMD_SLEEP_REQUEST`와 `POWERMANAGER_CMD_SLEEP_REQUEST`로 정리하더라도, 향후 실제 전원 종료 요청은 별도 명령으로 추가해야 한다는 원칙을 코드 주석과 문서에 남길 필요가 있다.

## 5. 다음 단계
- 필요 시 BLANK 상태명을 `SCREENMANAGER_STATE_POWER_OFF`로 유지할지 여부를 팀 용어 기준으로 최종 정리한다.
- 구현 후 변경 전/후 이름 매핑표를 문서와 함께 정리한다.
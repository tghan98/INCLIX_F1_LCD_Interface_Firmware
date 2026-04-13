# 2026-04-13 입력책임분리 InputInterpreter이관 phase01

## 1. 작업 목적
기존 테스트 목적의 BatteryDisplay 입력 소비 구조를 제거하고 실제 사용할 InputInterpreter 중심 구조로 이관한다. 또한 최소 Screen skeleton을 도입해 구조 이관 이후에도 화면 활성화가 정상 복구되는지 디버깅으로 확인한다.

## 2. 수행내용
### 2.1 InputInterpreter 도입
InputInterpreter App/Interface를 도입해 Button/Battery 원시 이벤트를 직접 소비하고, 의미 명령으로 변환해 PowerManager에 전달하는 경로를 구성했다. 버튼 이벤트는 사용자 활동 및 분석 시작 요청 계열로 번역되도록 정리했으며, 배터리 이벤트는 low/critical 상태를 PowerManager 명령으로 라우팅하도록 연결했다. AnalysisSequenceManager는 아직 본 구현 전 단계이므로 실제 실행 대신 TODO 연결점만 유지했다.

### 2.2 BatteryDisplay 경로 분리
User_Main에서 BatteryDisplay include와 Init/Run 호출을 제거해 배터리 입력 소비 책임을 InputInterpreter로 단일화했다. 이로써 BatteryDisplay와 InputInterpreter의 이중 소비 충돌을 제거하고, User_Main은 생산자 실행과 해석/정책 모듈 호출만 수행하는 스케줄러 구조를 유지하도록 정렬했다.

### 2.3 Screen skeleton 도입
화면 복구를 위해 ScreenManager App/Interface 최소 skeleton을 추가했다. BOOT에서 IDLE로 전이되는 최소 상태머신과 문자열 출력 경로를 구현하고, Init/Run을 User_Main 스케줄에 연결해 ScreenManager가 화면 활성화 책임을 담당하도록 분리했다.

## 3. 검증 결과
빌드가 정상 완료되었고, 실기 디버깅에서 INCLIX READY 문구가 화면에 정상 표시되는 것을 확인했다. 이는 입력 책임 이관 후에도 최소 Screen 경로를 통해 화면 활성화가 복구되었음을 의미한다.

## 4. 잔여 이슈 / TODO
AnalysisSequenceManager는 아직 skeleton 연결점만 존재하며 실제 명령 소비 경로가 미구현 상태다. PowerControl_App는 현재 비활성 상태로 유지 중이며, 후속 단계에서 HW 제어 전용 축소 또는 최종 제거 여부를 결정해야 한다. BatteryDisplay 파일은 런타임 경로에서 분리되었으므로 프로젝트 참조 정리 및 최종 삭제 여부를 후속 단계에서 확정한다.

## 5. 다음 단계
InputInterpreter의 분석 시작 요청 명령을 AnalysisSequenceManager skeleton으로 실제 전달하도록 연결한다. PowerManager와 AnalysisSequenceManager 사이의 상태 연계 규칙을 정의해 화면 정책 이벤트를 표준화한다. 마지막으로 BatteryDisplay 관련 파일/프로젝트 참조 정리 여부를 결정해 구조 정리를 완료한다.

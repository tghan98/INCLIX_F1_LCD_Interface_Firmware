# 2026-04-14_01_PowerManager_정책확장.md

## 1. 작업 목적
PowerManager를 skeleton 수준에서 실제 전원 정책의 단일 책임자로 확장한다. 경과시간 기반 상태 전이를 실제 값(10분/5분/3초)으로 적용하고, VBUS 감지를 통한 Charging/Discharging 자동 전환, 배터리 low/critical 반영, 실제 HW 전원 차단까지 구현한다.

## 2. 수행내용

### 2.1 enum 이름 통일
PowerManager와 ScreenManager 양쪽에 흩어져 있던 `POWER_OFF_PENDING`, `POWER_OFF_HINT` 계열 이름을 `POWER_OFF_NOTICE`로 통일했다. 이 작업은 이후 모든 전이 로직과 화면 전환 로직의 기반이 되므로 가장 먼저 수행했다. 관련 컨텍스트 구조체 필드명(`power_off_pending_timeout_ms`)도 함께 변경했다.

### 2.2 PowerManager 전이 로직 실제화
경과시간 상수를 실제 정책 값(STANDBY 무조작 10분, SLEEP 5분, POWER_OFF_NOTICE 3초)으로 수정했다. SLEEP에서 POWER_OFF_NOTICE로 전이되는 조건에 DISCHARGING 모드 제한을 추가해 충전 중에는 자동 전원 차단이 발생하지 않도록 했다. CHARGER_ATTACHED 수신 시 SLEEP 상태이면 STANDBY로 wakeup하도록 강화했다. POWER_OFF 상태 진입 시 `PowerControl_Drv_WriteHold(GPIO_PIN_RESET)`을 일회성으로 호출해 실제 HW 전원을 차단하도록 구현했다. 초기화 시 VBUS 상태를 읽어 mode를 CHARGING 또는 DISCHARGING으로 설정해 부팅 직후 UNKNOWN 상태로 인한 전원 차단 불가 문제를 방지했다.

### 2.3 PowerControl_App 제거 및 Hold 이관
테스트 목적으로 작성된 PowerControl_App을 삭제하고, Hold 핀 제어 책임을 PowerManager로 이관했다. PowerManager_App_Init()에서 `PowerControl_Drv_WriteHold(GPIO_PIN_SET)`을 호출해 부팅 시 전원 유지 핀을 확보한다. PowerControl_Drv는 GPIO 접근 전용 드라이버로 유지하며, PowerManager와 InputInterpreter가 직접 사용한다. 보드에서 LED가 제거된 상태이므로 LED 관련 처리는 전면 제외했다. IAR 프로젝트 파일(.ewp)에서 PowerControl_App include 경로와 파일 그룹도 함께 제거했다.

### 2.4 InputInterpreter VBUS 감지 추가
InputInterpreter에 `PollVbusEvents()` 함수를 추가해 `PowerControl_Drv_ReadUsbDetect()` 상태 변화를 감지하고 CHARGER_ATTACHED / CHARGER_DETACHED 명령으로 변환해 PowerManager에 전달하도록 연결했다. 초기화 시 현재 VBUS 상태를 기록해 첫 폴링에서 spurious 이벤트가 발생하지 않도록 했다. 이 구현은 임시 처리로, 장기적으로는 VbusMonitor_Interface 별도 계층으로 분리가 필요하다.

### 2.5 ScreenManager 자동 화면 전환
ScreenManager가 매 Run마다 PowerManager 상태를 폴링해 변화를 감지하고 자동으로 화면을 전환하도록 구현했다. PM 상태별 화면 매핑은 STANDBY→IDLE("INCLIX READY"), SLEEP→화면 소거, POWER_OFF_NOTICE→"TURNING OFF...", POWER_OFF→화면 소거로 구성했다. IDLE 화면에서는 `battery_low_latched` 플래그를 확인해 저전압 상태일 때 "BAT LOW" 텍스트를 추가로 표시한다.

## 3. 검증 결과
- [x] IAR 빌드 통과 (링크 에러, 미사용 enum 경고 없음)
- [x] 실기: 부팅 → BOOT 화면 → READY(Standby) 화면 자동 전환
- [x] 실기: 무조작 10분 → SLEEP 화면 (텍스트 소거)
- [x] 실기: SLEEP 화면에서 버튼 → READY 복귀

## 4. 잔여 이슈/TODO
- `InputInterpreter_DispatchToAnalysisSkeleton()`이 아직 AnalysisSequenceManager에 연결되지 않은 상태다.
- Button_Drv가 `BUTTON_EVENT_LONG_PRESS`를 제공하지 않아 long-press 기능을 구현하지 못했다. Button 계층에 long-press 이벤트를 추가한 뒤 InputInterpreter를 연결해야 한다.
- `BATTERY_CRITICAL` 수신 시 즉시 POWER_OFF_NOTICE로 진입할지 여부는 배터리 특성 검토 후 별도로 결정해야 한다. 현재는 latch 기록까지만 반영했다.
- 현재 InputInterpreter가 VBUS를 직접 폴링하고 있어, 장기적으로는 VbusMonitor_Interface를 별도 계층으로 분리해야 한다.
- 충전 연결만으로 `battery_low_latched`를 즉시 clear하지 않는다. BatteryMonitor 재측정 결과를 기반으로 갱신하는 정책을 별도로 결정해야 한다.
- DISCHARGING 상태에서 SLEEP timeout 후 "TURNING OFF" 표시와 실제 전원 차단 흐름을 아직 실기에서 확인하지 않았다. 경과시간을 임시로 축소한 뒤 확인이 필요하다.
- CHARGING 모드에서 SLEEP timeout이 경과해도 전원이 유지되는지 실기에서 확인하지 않았다. 경과시간을 임시로 축소한 뒤 확인이 필요하다.
- USB 연결·분리 시 Charging/Discharging 전환이 화면에 반영되는지 실기에서 확인하지 않았다. 디버거 또는 화면 임시 표시를 통해 확인이 필요하다.
- IDLE 화면에서 배터리 저전압 시 "BAT LOW" 텍스트가 표시되는지 실기에서 확인하지 않았다. BatteryMonitor 임계값을 임시로 상향한 뒤 확인이 필요하다.

## 5. 다음 단계
- AnalysisSequenceManager를 도입하고 InputInterpreter와 연결한다.
- 배터리 레벨별 아이콘 등 PowerManager ↔ ScreenManager 간 상태 연계 규칙을 고도화한다.

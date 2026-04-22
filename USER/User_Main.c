#include "User_Main.h"

#include "Button_Interface.h"
#include "BatteryMonitor_Interface.h"
#include "App/CodeChip_App/CodeChip_Interface.h"
#include "App/InputInterpreter_App/InputInterpreter_Interface.h"
#include "App/SequenceManager_App/SequenceManager_Interface.h"
#include "App/PowerManager_App/PowerManager_Interface.h"
#include "App/ScreenManager_App/ScreenManager_Interface.h"
#include "User_HAL_Drv.h"

/**
 * @brief  시스템 전체 초기화 진입점.
 * @details 보드 HAL 래퍼 → 생산자(Button/BatteryMonitor) → 해석기(InputInterpreter)
 *          → 검사 절차(SequenceManager) → 정책(PowerManager) → 화면(ScreenManager)
 *          순서로 초기화한다.
 *          순서 변경 시 의존 관계가 깨질 수 있으므로 주의한다.
 * @return  0: 정상 완료
 */
int32_t User_Main_Init(void)
{
  /* 1) 보드 의존 HAL 래퍼 초기화 — 클럭, GPIO 등 하드웨어 기반 설정 */
  (void)UserHAL_Config();

  /* 2) 버튼 인터페이스 초기화 — 이후 Run에서 이벤트 큐를 채우는 생산자 역할 */
  Button_Interface_Init();

  /* 3) 배터리 모니터링 초기화 — ADC 기반 레벨 측정 생산자 */
  BatteryMonitor_Interface_Init();

  /* 4) CodeChip 인터페이스 초기화 — SD card detect GPIO 폴링 생산자 */
  CodeChip_Interface_Init();

  /* 5) 입력 해석기 초기화 — Button/Battery/CodeChip 이벤트를 의미 명령으로 변환 */
  InputInterpreter_Interface_Init();

  /* 5) 검사 절차 상태머신 초기화 — 분석 skeleton 상태 owner */
  SequenceManager_Interface_Init();

  /* 6) 전원 정책 상태머신 초기화 — BOOT 상태에서 시작 */
  PowerManager_Interface_Init();

  /* 7) 화면 상태머신 초기화 — BOOT 화면 표시 후 장면 조합 준비 */
  ScreenManager_Interface_Init();

  /* PowerControl_App_Init() 은 OLED 슬라이드쇼 검증 완료 전까지 비활성화 */
  /* PowerControl_App_Init(); */

  return 0;
}

/**
 * @brief  메인 루프 1회 실행 — 생산자 → 해석 → 검사절차 → 정책 → 화면 순서 고정.
 * @details 각 모듈은 독립적인 큐/상태를 통해 통신하며, 호출 순서가
 *          데이터 흐름 방향을 결정한다. 순서를 바꾸지 않는다.
 * @return  0: 정상 완료
 */
int32_t User_Main_Run(void)
{
  /* 1) 버튼 스캔 — 물리 입력을 읽어 이벤트 큐에 채움 (최우선 실행) */
  Button_Interface_Run();

  /* 2) 배터리 레벨 측정 — ADC 결과를 이벤트로 변환하여 큐에 채움 */
  BatteryMonitor_Interface_Run();

  /* 3) CodeChip 감지 — SD detect GPIO를 폴링하여 삽입/제거 이벤트를 큐에 쉡 */
  CodeChip_Interface_Run();

  /* 4) 입력 해석 — Button/Battery/CodeChip 이벤트를 소비하여 Power/Analysis 명령으로 변환 */
  InputInterpreter_Interface_Run();

  /* 4) 검사 절차 실행 — 분석 시작 요청과 절차 상태 전이를 반영 */
  SequenceManager_Interface_Run();

  /* 5) 전원 정책 실행 — 명령 큐 소비 후 전원 상태머신 한 틱 실행 */
  PowerManager_Interface_Run();

  /* 6) 화면 상태머신 실행 — 전원축과 검사축을 조합해 렌더링 수행 */
  ScreenManager_Interface_Run();

  return 0;
}

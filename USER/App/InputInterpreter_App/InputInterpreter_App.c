/* Input interpreter app (internal logic) */

#include "InputInterpreter_App.h"

#include "App/Button_App/Button_Interface.h"
#include "App/BatteryMonitor_App/BatteryMonitor_Interface.h"
#include "App/CodeChip_App/CodeChip_Interface.h"
#include "App/PowerManager_App/PowerManager_Interface.h"
#include "App/SequenceManager_App/SequenceManager_Interface.h"

#include "BatteryMonitor_App.h"
#include "Button_Drv.h"
#include "PowerControl_Drv.h"

#include <string.h>

static InputInterpreter_TranslatedCmd_t s_last_translated_cmd;
static GPIO_PinState s_prev_vbus_state;

static void InputInterpreter_ResetLastTranslated(void);
static int32_t InputInterpreter_DispatchToPower(const InputInterpreter_TranslatedCmd_t* cmd);
static int32_t InputInterpreter_DispatchToAnalysis(const InputInterpreter_TranslatedCmd_t* cmd);
static int32_t InputInterpreter_Dispatch(const InputInterpreter_TranslatedCmd_t* cmd);
static void InputInterpreter_TranslateButtonEvent(const ButtonAppEvent_t* event);
static void InputInterpreter_TranslateBatteryEvent(const BatteryMonitor_AppEvent_t* event);
static void InputInterpreter_TranslateCodeChipEvent(const CodeChip_AppEvent_t* event);
static void InputInterpreter_PollButtonEvents(void);
static void InputInterpreter_PollBatteryEvents(void);
static void InputInterpreter_PollCodeChipEvents(void);
static void InputInterpreter_PollVbusEvents(void);

/**
 * @brief 마지막 변환 명령 정보를 초기값으로 리셋한다.
 */
static void InputInterpreter_ResetLastTranslated(void)
{
  memset(&s_last_translated_cmd, 0, sizeof(s_last_translated_cmd));
  s_last_translated_cmd.target = INPUTINTERPRETER_TARGET_NONE;
  s_last_translated_cmd.cmd = INPUTINTERPRETER_CMD_NONE;
}

/**
 * @brief Power 도메인으로 보낼 명령으로 변환해 전달한다.
 * @param cmd 변환된 입력 명령.
 * @return 성공 시 0, 입력 포인터가 NULL이면 -1, 큐 오류 시 음수.
 */
static int32_t InputInterpreter_DispatchToPower(const InputInterpreter_TranslatedCmd_t* cmd)
{
  PowerManager_Command_t power_cmd;

  if (cmd == NULL)
  {
    return -1;
  }

  switch (cmd->cmd)
  {
    case INPUTINTERPRETER_CMD_USER_ACTIVITY:
      power_cmd = POWERMANAGER_CMD_ACTIVITY;
      break;

    case INPUTINTERPRETER_CMD_POWER_BATTERY_LOW:
      power_cmd = POWERMANAGER_CMD_BATTERY_LOW;
      break;

    case INPUTINTERPRETER_CMD_POWER_BATTERY_CRITICAL:
      power_cmd = POWERMANAGER_CMD_BATTERY_CRITICAL;
      break;

    case INPUTINTERPRETER_CMD_CHARGER_ATTACHED:
      power_cmd = POWERMANAGER_CMD_CHARGER_ATTACHED;
      break;

    case INPUTINTERPRETER_CMD_CHARGER_DETACHED:
      power_cmd = POWERMANAGER_CMD_CHARGER_DETACHED;
      break;

    case INPUTINTERPRETER_CMD_SLEEP_REQUEST:
      power_cmd = POWERMANAGER_CMD_SLEEP_REQUEST;
      break;

    default:
      return 0;
  }

  return PowerManager_Interface_SubmitCommand(power_cmd);
}

/**
 * @brief Analysis 도메인 명령을 SequenceManager 명령으로 변환해 전달한다.
 * @param cmd 변환된 입력 명령.
 * @return 성공 시 0, 입력 포인터가 NULL이면 -1, 큐 오류 시 음수.
 */
static int32_t InputInterpreter_DispatchToAnalysis(const InputInterpreter_TranslatedCmd_t* cmd)
{
  SequenceManager_Command_t sequence_cmd;
  SequenceManager_State_t seq_state;

  if (cmd == NULL)
  {
    return -1;
  }

  seq_state = SequenceManager_Interface_GetState();

  switch (cmd->cmd)
  {
    case INPUTINTERPRETER_CMD_ANALYSIS_START_REQUEST:
      if (seq_state == SEQUENCEMANAGER_STATE_READY_TO_INCUBATE)
      {
        return 0;
      }
      sequence_cmd = SEQUENCEMANAGER_CMD_START_REQUEST;
      break;

    case INPUTINTERPRETER_CMD_CODECHIP_INSERTED:
      sequence_cmd = SEQUENCEMANAGER_CMD_CODECHIP_INSERTED;
      break;

    case INPUTINTERPRETER_CMD_CASSETTE_INSERTED:
      sequence_cmd = SEQUENCEMANAGER_CMD_CASSETTE_INSERTED;
      break;

    default:
      return 0;
  }

  return SequenceManager_Interface_SubmitCommand(sequence_cmd);
}

/**
 * @brief 명령 target에 따라 Power 또는 Analysis로 라우팅한다.
 * @param cmd 라우팅할 변환 명령.
 * @return 각 도메인 dispatch 결과.
 */
static int32_t InputInterpreter_Dispatch(const InputInterpreter_TranslatedCmd_t* cmd)
{
  int32_t result;

  if (cmd == NULL)
  {
    return -1;
  }

  if (cmd->target == INPUTINTERPRETER_TARGET_POWER)
  {
    result = InputInterpreter_DispatchToPower(cmd);
  }
  else if (cmd->target == INPUTINTERPRETER_TARGET_ANALYSIS)
  {
    result = InputInterpreter_DispatchToAnalysis(cmd);
  }
  else
  {
    result = 0;
  }

  s_last_translated_cmd = *cmd;
  return result;
}

/**
 * @brief 버튼 원시 이벤트를 의미 명령으로 변환한다.
 * @param event Button 모듈에서 받은 이벤트.
 */
static void InputInterpreter_TranslateButtonEvent(const ButtonAppEvent_t* event)
{
  InputInterpreter_TranslatedCmd_t cmd;
  SequenceManager_State_t seq_state;

  if (event == NULL)
  {
    return;
  }

  if (event->button_id != BUTTON_ID_PAUSE)
  {
    return;
  }

  memset(&cmd, 0, sizeof(cmd));
  cmd.timestamp_ms = HAL_GetTick();
  seq_state = SequenceManager_Interface_GetState();

  if (event->event == BUTTON_EVENT_CLICK)
  {
    cmd.target = INPUTINTERPRETER_TARGET_POWER;
    cmd.cmd = INPUTINTERPRETER_CMD_USER_ACTIVITY;
    (void)InputInterpreter_Dispatch(&cmd);

    if (seq_state == SEQUENCEMANAGER_STATE_WAIT_CASSETTE)
    {
      /* TEMP STUB: cassette hardware is not available yet, so button click becomes cassette inserted. */
      cmd.target = INPUTINTERPRETER_TARGET_ANALYSIS;
      cmd.cmd = INPUTINTERPRETER_CMD_CASSETTE_INSERTED;
      (void)InputInterpreter_Dispatch(&cmd);
    }
    else if (seq_state != SEQUENCEMANAGER_STATE_READY_TO_INCUBATE)
    {
      cmd.target = INPUTINTERPRETER_TARGET_ANALYSIS;
      cmd.cmd = INPUTINTERPRETER_CMD_ANALYSIS_START_REQUEST;
      (void)InputInterpreter_Dispatch(&cmd);
    }
  }
  else if (event->event == BUTTON_EVENT_PRESSED)
  {
    cmd.target = INPUTINTERPRETER_TARGET_POWER;
    cmd.cmd = INPUTINTERPRETER_CMD_USER_ACTIVITY;
    (void)InputInterpreter_Dispatch(&cmd);

    /* TODO: map long-press to INPUTINTERPRETER_CMD_SLEEP_REQUEST when supported */
  }
  else
  {
    /* BUTTON_EVENT_RELEASED and others are ignored in phase 1 */
  }
}

/**
 * @brief 배터리 원시 이벤트를 명령으로 변환한다.
 * @param event BatteryMonitor 모듈에서 받은 이벤트.
 */
static void InputInterpreter_TranslateBatteryEvent(const BatteryMonitor_AppEvent_t* event)
{
  InputInterpreter_TranslatedCmd_t cmd;

  /* 방어 코드: 잘못된 입력(NULL)은 즉시 무시한다. */
  if (event == NULL)
  {
    return;
  }

  /*
   * 1) 배터리 이벤트를 Power 도메인 명령으로 바꾸기 위한 기본 정보를 먼저 채운다.
   *    - target      : 이 명령의 목적지를 PowerManager로 지정
   *    - timestamp   : 이벤트를 해석한 시각 기록(디버깅/추적용)
   *    - param0      : 원본 배터리 레벨을 그대로 보관
   */
  memset(&cmd, 0, sizeof(cmd));
  cmd.target = INPUTINTERPRETER_TARGET_POWER;
  cmd.timestamp_ms = HAL_GetTick();
  cmd.param0 = event->level;

  /*
   * 2) phase 1 정책에서는 LOW/CRITICAL만 PowerManager에 전달한다.
   *    - CRITICAL: 즉시 대응이 필요한 위험 상태
   *    - LOW     : 저전압 상태(추가 정책 판단의 입력)
   */
  if (event->level == BATTERY_LEVEL_CRITICAL)
  {
    cmd.cmd = INPUTINTERPRETER_CMD_POWER_BATTERY_CRITICAL;
    (void)InputInterpreter_Dispatch(&cmd);
  }
  else if (event->level == BATTERY_LEVEL_LOW)
  {
    cmd.cmd = INPUTINTERPRETER_CMD_POWER_BATTERY_LOW;
    (void)InputInterpreter_Dispatch(&cmd);
  }
  else
  {
    /*
     * 3) MEDIUM/HIGH는 정상 범주로 보고 phase 1에서는 정책 명령을 만들지 않는다.
     *    필요해지면 이후 단계에서 이 구간에 명령 매핑을 추가하면 된다.
     */
  }
}

/**
 * @brief CodeChip 원시 이벤트를 의미 명령으로 변환한다.
 * @param event CodeChip_Interface에서 받은 이벤트.
 */
static void InputInterpreter_TranslateCodeChipEvent(const CodeChip_AppEvent_t* event)
{
  InputInterpreter_TranslatedCmd_t cmd;

  if (event == NULL)
  {
    return;
  }

  memset(&cmd, 0, sizeof(cmd));
  cmd.target       = INPUTINTERPRETER_TARGET_ANALYSIS;
  cmd.timestamp_ms = HAL_GetTick();

  if (event->event == CODECHIP_EVENT_INSERTED)
  {
    cmd.cmd = INPUTINTERPRETER_CMD_CODECHIP_INSERTED;
    (void)InputInterpreter_Dispatch(&cmd);
  }
  else
  {
    /* 이번 단계에서 제거 이벤트는 SequenceManager 명령으로 변환하지 않는다. */
  }
}

/**
 * @brief CodeChip 이벤트 큐를 모두 읽어 변환 처리한다.
 */
static void InputInterpreter_PollCodeChipEvents(void)
{
  CodeChip_AppEvent_t event;

  while (CodeChip_Interface_GetEvent(&event) == 0)
  {
    InputInterpreter_TranslateCodeChipEvent(&event);
  }
}

/**
 * @brief 버튼 이벤트 큐를 모두 읽어 변환 처리한다.
 */
static void InputInterpreter_PollButtonEvents(void)
{
  ButtonAppEvent_t event;

  while (Button_Interface_GetEvent(&event) == 0)
  {
    InputInterpreter_TranslateButtonEvent(&event);
  }
}

/**
 * @brief 배터리 이벤트 큐를 모두 읽어 변환 처리한다.
 */
static void InputInterpreter_PollBatteryEvents(void)
{
  BatteryMonitor_AppEvent_t event;

  while (BatteryMonitor_Interface_GetEvent(&event) == 0)
  {
    InputInterpreter_TranslateBatteryEvent(&event);
  }
}

/**
 * @brief VBUS 상태 변화를 감지해 Charger Attached/Detached 명령으로 변환한다.
 * @details 임시 구현: InputInterpreter가 직접 GPIO 폴링.
 *          장기적으로 VbusMonitor_Interface 별도 계층으로 분리 필요.
 */
static void InputInterpreter_PollVbusEvents(void)
{
  GPIO_PinState cur_vbus;
  InputInterpreter_TranslatedCmd_t cmd;

  cur_vbus = PowerControl_Drv_ReadUsbDetect();

  if (cur_vbus == s_prev_vbus_state)
  {
    return;
  }

  s_prev_vbus_state = cur_vbus;

  memset(&cmd, 0, sizeof(cmd));
  cmd.target = INPUTINTERPRETER_TARGET_POWER;
  cmd.timestamp_ms = HAL_GetTick();

  if (cur_vbus == GPIO_PIN_SET)
  {
    cmd.cmd = INPUTINTERPRETER_CMD_CHARGER_ATTACHED;
  }
  else
  {
    cmd.cmd = INPUTINTERPRETER_CMD_CHARGER_DETACHED;
  }

  (void)InputInterpreter_Dispatch(&cmd);
}

/**
 * @brief InputInterpreter 내부 상태를 초기화한다.
 * @return 성공 시 0.
 */
int32_t InputInterpreter_App_Init(void)
{
  InputInterpreter_ResetLastTranslated();
  /* 초기화 시 현재 VBUS 상태를 기록해 첫 폴링에서 가짜 이벤트 방지 */
  s_prev_vbus_state = PowerControl_Drv_ReadUsbDetect();
  return 0;
}

/**
 * @brief 입력 이벤트를 수집하고 의미 명령으로 변환해 전달한다.
 * @return 성공 시 0.
 */
int32_t InputInterpreter_App_Run(void)
{
  InputInterpreter_PollButtonEvents();
  InputInterpreter_PollBatteryEvents();
  InputInterpreter_PollCodeChipEvents();
  InputInterpreter_PollVbusEvents();
  return 0;
}

/**
 * @brief 마지막으로 변환된 명령 스냅샷을 조회한다.
 * @param out_cmd 결과를 받을 출력 포인터.
 * @return 성공 시 0, out_cmd가 NULL이면 -1.
 */
int32_t InputInterpreter_App_GetLastTranslated(InputInterpreter_TranslatedCmd_t* out_cmd)
{
  if (out_cmd == NULL)
  {
    return -1;
  }

  *out_cmd = s_last_translated_cmd;
  return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

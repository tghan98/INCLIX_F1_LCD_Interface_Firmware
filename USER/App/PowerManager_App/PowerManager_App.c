/* Power manager app (internal logic) */

#include "PowerManager_App.h"
#include <string.h>

#define POWERMANAGER_DEFAULT_IDLE_TIMEOUT_MS              (30000U)
#define POWERMANAGER_DEFAULT_SLEEP_TIMEOUT_MS             (60000U)
#define POWERMANAGER_DEFAULT_POWER_OFF_PENDING_TIMEOUT_MS (5000U)
#define POWERMANAGER_CMD_QUEUE_SIZE                       (8U)

typedef struct
{
  PowerManager_Command_t cmd_buffer[POWERMANAGER_CMD_QUEUE_SIZE];
  uint32_t head;
  uint32_t tail;
  uint32_t count;
} PowerManager_CmdQueue_t;

static PowerManager_Context_t s_pm_ctx;
static PowerManager_CmdQueue_t s_pm_cmd_q;

static void PowerManager_CmdQueue_Init(void);
static int32_t PowerManager_CmdQueue_Push(PowerManager_Command_t cmd);
static int32_t PowerManager_CmdQueue_Pop(PowerManager_Command_t* out_cmd);
static void PowerManager_TransitionState(PowerManager_State_t next_state, uint32_t now_tick);
static void PowerManager_HandleCommand(PowerManager_Command_t cmd, uint32_t now_tick);
static void PowerManager_RunStateMachine(uint32_t now_tick);

/**
 * @brief 내부 명령 큐를 초기화한다.
 */
static void PowerManager_CmdQueue_Init(void)
{
  memset(&s_pm_cmd_q, 0, sizeof(s_pm_cmd_q));
}

/**
 * @brief 내부 명령 큐에 명령을 추가한다.
 * @param cmd 큐에 추가할 명령.
 * @return 성공 시 0, 큐가 가득 찬 경우 -1.
 */
static int32_t PowerManager_CmdQueue_Push(PowerManager_Command_t cmd)
{
  if (s_pm_cmd_q.count >= POWERMANAGER_CMD_QUEUE_SIZE)
  {
    return -1;
  }

  s_pm_cmd_q.cmd_buffer[s_pm_cmd_q.tail] = cmd;
  s_pm_cmd_q.tail = (s_pm_cmd_q.tail + 1U) % POWERMANAGER_CMD_QUEUE_SIZE;
  s_pm_cmd_q.count++;
  return 0;
}

/**
 * @brief 내부 명령 큐에서 명령 1개를 꺼낸다.
 * @param out_cmd 꺼낸 명령을 저장할 출력 포인터.
 * @return 성공 시 0, 포인터 오류 또는 큐가 비어 있으면 -1.
 */
static int32_t PowerManager_CmdQueue_Pop(PowerManager_Command_t* out_cmd)
{
  if ((out_cmd == NULL) || (s_pm_cmd_q.count == 0U))
  {
    return -1;
  }

  *out_cmd = s_pm_cmd_q.cmd_buffer[s_pm_cmd_q.head];
  s_pm_cmd_q.head = (s_pm_cmd_q.head + 1U) % POWERMANAGER_CMD_QUEUE_SIZE;
  s_pm_cmd_q.count--;
  return 0;
}

/**
 * @brief 다음 전원 상태로 전이하고 상태 진입 시각을 갱신한다.
 * @param next_state 전이할 목표 상태.
 * @param now_tick 현재 시스템 tick(ms).
 */
static void PowerManager_TransitionState(PowerManager_State_t next_state, uint32_t now_tick)
{
  s_pm_ctx.state = next_state;
  s_pm_ctx.state_enter_tick = now_tick;
}

/**
 * @brief 외부/내부 명령 1개를 처리해 컨텍스트와 상태를 갱신한다.
 * @param cmd 처리할 명령.
 * @param now_tick 현재 시스템 tick(ms).
 */
static void PowerManager_HandleCommand(PowerManager_Command_t cmd, uint32_t now_tick)
{
  switch (cmd)
  {
    case POWERMANAGER_CMD_ACTIVITY:
      s_pm_ctx.last_activity_tick = now_tick;
      if (s_pm_ctx.state == POWERMANAGER_STATE_SLEEP)
      {
        PowerManager_TransitionState(POWERMANAGER_STATE_STANDBY, now_tick);
      }
      break;

    case POWERMANAGER_CMD_FORCE_SLEEP:
      PowerManager_TransitionState(POWERMANAGER_STATE_SLEEP, now_tick);
      break;

    case POWERMANAGER_CMD_WAKEUP:
      PowerManager_TransitionState(POWERMANAGER_STATE_STANDBY, now_tick);
      s_pm_ctx.last_activity_tick = now_tick;
      break;

    case POWERMANAGER_CMD_BATTERY_LOW:
      s_pm_ctx.battery_low_latched = 1U;
      /* 정책 세부 조건은 InputInterpreter 연계 단계에서 확정 */
      break;

    case POWERMANAGER_CMD_BATTERY_CRITICAL:
      s_pm_ctx.battery_critical_latched = 1U;
      PowerManager_TransitionState(POWERMANAGER_STATE_POWER_OFF_PENDING, now_tick);
      break;

    case POWERMANAGER_CMD_CHARGER_ATTACHED:
      s_pm_ctx.mode = POWERMANAGER_MODE_CHARGING;
      break;

    case POWERMANAGER_CMD_CHARGER_DETACHED:
      s_pm_ctx.mode = POWERMANAGER_MODE_DISCHARGING;
      break;

    case POWERMANAGER_CMD_NONE:
    default:
      break;
  }
}

/**
 * @brief 경과 시간 기준으로 전원 상태머신을 한 스텝 실행한다.
 * @param now_tick 현재 시스템 tick(ms).
 */
static void PowerManager_RunStateMachine(uint32_t now_tick)
{
  uint32_t elapsed_idle;
  uint32_t elapsed_state;

  elapsed_idle = now_tick - s_pm_ctx.last_activity_tick;
  elapsed_state = now_tick - s_pm_ctx.state_enter_tick;

  switch (s_pm_ctx.state)
  {
    case POWERMANAGER_STATE_BOOT:
      PowerManager_TransitionState(POWERMANAGER_STATE_STANDBY, now_tick);
      break;

    case POWERMANAGER_STATE_STANDBY:
      if (elapsed_idle >= s_pm_ctx.idle_timeout_ms)
      {
        PowerManager_TransitionState(POWERMANAGER_STATE_SLEEP, now_tick);
      }
      break;

    case POWERMANAGER_STATE_SLEEP:
      if (elapsed_state >= s_pm_ctx.sleep_timeout_ms)
      {
        PowerManager_TransitionState(POWERMANAGER_STATE_POWER_OFF_PENDING, now_tick);
      }
      break;

    case POWERMANAGER_STATE_POWER_OFF_PENDING:
      if (elapsed_state >= s_pm_ctx.power_off_pending_timeout_ms)
      {
        PowerManager_TransitionState(POWERMANAGER_STATE_POWER_OFF, now_tick);
      }
      break;

    case POWERMANAGER_STATE_POWER_OFF:
      /* TODO: 실제 HW power hold 연동은 후속 단계에서 구현 */
      break;

    default:
      PowerManager_TransitionState(POWERMANAGER_STATE_BOOT, now_tick);
      break;
  }
}

/**
 * @brief PowerManager 컨텍스트와 명령 큐를 초기화한다.
 * @return 성공 시 0.
 */
int32_t PowerManager_App_Init(void)
{
  uint32_t now_tick;

  memset(&s_pm_ctx, 0, sizeof(s_pm_ctx));
  PowerManager_CmdQueue_Init();

  now_tick = HAL_GetTick();

  s_pm_ctx.state = POWERMANAGER_STATE_BOOT;
  s_pm_ctx.mode = POWERMANAGER_MODE_UNKNOWN;
  s_pm_ctx.state_enter_tick = now_tick;
  s_pm_ctx.last_activity_tick = now_tick;
  s_pm_ctx.idle_timeout_ms = POWERMANAGER_DEFAULT_IDLE_TIMEOUT_MS;
  s_pm_ctx.sleep_timeout_ms = POWERMANAGER_DEFAULT_SLEEP_TIMEOUT_MS;
  s_pm_ctx.power_off_pending_timeout_ms = POWERMANAGER_DEFAULT_POWER_OFF_PENDING_TIMEOUT_MS;

  return 0;
}

/**
 * @brief 명령 처리와 상태머신 실행을 1주기 수행한다.
 * @return 성공 시 0.
 */
int32_t PowerManager_App_Run(void)
{
  uint32_t now_tick;
  PowerManager_Command_t cmd;

  now_tick = HAL_GetTick();

  while (PowerManager_CmdQueue_Pop(&cmd) == 0)
  {
    PowerManager_HandleCommand(cmd, now_tick);
  }

  PowerManager_RunStateMachine(now_tick);

  return 0;
}

/**
 * @brief PowerManager 명령 큐에 명령을 제출한다.
 * @param cmd 제출할 명령.
 * @return 성공 시 0, 큐가 가득 찬 경우 -1.
 */
int32_t PowerManager_App_SubmitCommand(PowerManager_Command_t cmd)
{
  return PowerManager_CmdQueue_Push(cmd);
}

/**
 * @brief 현재 PowerManager 상태를 조회한다.
 * @return 현재 PowerManager 상태.
 */
PowerManager_State_t PowerManager_App_GetState(void)
{
  return s_pm_ctx.state;
}

/**
 * @brief 현재 PowerManager 모드를 조회한다.
 * @return 현재 PowerManager 모드.
 */
PowerManager_Mode_t PowerManager_App_GetMode(void)
{
  return s_pm_ctx.mode;
}

/**
 * @brief 현재 PowerManager 컨텍스트를 호출자 버퍼로 복사한다.
 * @param out_ctx 컨텍스트 스냅샷을 받을 출력 포인터.
 * @return 성공 시 0, out_ctx가 NULL이면 -1.
 */
int32_t PowerManager_App_GetContext(PowerManager_Context_t* out_ctx)
{
  if (out_ctx == NULL)
  {
    return -1;
  }

  *out_ctx = s_pm_ctx;
  return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

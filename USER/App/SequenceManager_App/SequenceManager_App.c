/* Sequence manager app (internal logic) */

#include "SequenceManager_App.h"

#include "App/PowerManager_App/PowerManager_Interface.h"

/* SequenceManager 내부 명령 큐 최대 크기 */
#define SEQUENCEMANAGER_CMD_QUEUE_SIZE  8U

/* SequenceManager 명령 큐 관리 구조체 */
typedef struct
{
  SequenceManager_Command_t buffer[SEQUENCEMANAGER_CMD_QUEUE_SIZE];
  uint32_t head;
  uint32_t tail;
  uint32_t count;
} SequenceManager_CmdQueue_t;

/* 현재 검사 절차 상태 */
static SequenceManager_State_t s_state;
/* 현재 상태 진입 시각 */
static uint32_t s_state_enter_tick;
/* 전원 복귀 후 반영할 시작 요청 보관 플래그 */
static uint8_t s_pending_start_request;
/* 내부 명령 큐 인스턴스 */
static SequenceManager_CmdQueue_t s_cmd_queue;

static void SequenceManager_CmdQueue_Init(void);
static int32_t SequenceManager_CmdQueue_Push(SequenceManager_Command_t cmd);
static int32_t SequenceManager_CmdQueue_Pop(SequenceManager_Command_t* out_cmd);
static void SequenceManager_SetState(SequenceManager_State_t next_state);
static uint8_t SequenceManager_IsDisplayCapableState(PowerManager_State_t power_state);
static void SequenceManager_HandleCommand(SequenceManager_Command_t cmd);

/**
 * @brief 내부 명령 큐 인덱스와 개수를 초기화한다.
 */
static void SequenceManager_CmdQueue_Init(void)
{
  s_cmd_queue.head = 0U;
  s_cmd_queue.tail = 0U;
  s_cmd_queue.count = 0U;
}

/**
 * @brief 명령 큐에 새 SequenceManager 명령을 저장한다.
 * @param cmd 저장할 SequenceManager 명령.
 * @retval 0 저장 성공.
 * @retval -1 큐가 가득 차서 저장 실패.
 */
static int32_t SequenceManager_CmdQueue_Push(SequenceManager_Command_t cmd)
{
  if (s_cmd_queue.count >= SEQUENCEMANAGER_CMD_QUEUE_SIZE)
  {
    return -1;
  }

  s_cmd_queue.buffer[s_cmd_queue.tail] = cmd;
  s_cmd_queue.tail = (s_cmd_queue.tail + 1U) % SEQUENCEMANAGER_CMD_QUEUE_SIZE;
  s_cmd_queue.count++;

  return 0;
}

/**
 * @brief 명령 큐에서 다음 SequenceManager 명령을 하나 꺼낸다.
 * @param out_cmd 꺼낸 명령을 저장할 출력 포인터.
 * @retval 0 명령 읽기 성공.
 * @retval -1 출력 포인터가 NULL이거나 큐가 비어 있음.
 */
static int32_t SequenceManager_CmdQueue_Pop(SequenceManager_Command_t* out_cmd)
{
  if ((out_cmd == NULL) || (s_cmd_queue.count == 0U))
  {
    return -1;
  }

  *out_cmd = s_cmd_queue.buffer[s_cmd_queue.head];
  s_cmd_queue.head = (s_cmd_queue.head + 1U) % SEQUENCEMANAGER_CMD_QUEUE_SIZE;
  s_cmd_queue.count--;

  return 0;
}

/**
 * @brief 현재 검사 절차 상태를 변경하고 진입 시각을 기록한다.
 * @param next_state 전이할 다음 상태.
 */
static void SequenceManager_SetState(SequenceManager_State_t next_state)
{
  s_state = next_state;
  s_state_enter_tick = HAL_GetTick();
}

/**
 * @brief 현재 전원 상태가 검사 화면 표시가 가능한 상태인지 확인한다.
 * @param power_state 확인할 PowerManager 상태.
 * @retval 1 표시 가능 상태.
 * @retval 0 표시 불가 상태.
 */
static uint8_t SequenceManager_IsDisplayCapableState(PowerManager_State_t power_state)
{
  return (power_state == POWERMANAGER_STATE_STANDBY) ? 1U : 0U;
}

/**
 * @brief 수신한 명령에 따라 검사 절차 상태 전이를 수행한다.
 * @details 시작 요청은 현재 상태가 IDLE일 때만 반영한다. 전원 상태가 아직
 *          표시 가능 상태가 아니면 즉시 진행하지 않고 pending 플래그에 보관한다.
 * @param cmd 처리할 SequenceManager 명령.
 */
static void SequenceManager_HandleCommand(SequenceManager_Command_t cmd)
{
  PowerManager_State_t power_state;

  power_state = PowerManager_Interface_GetState();

  switch (cmd)
  {
    case SEQUENCEMANAGER_CMD_START_REQUEST:
      if (s_state != SEQUENCEMANAGER_STATE_IDLE)
      {
        break;
      }

      if (SequenceManager_IsDisplayCapableState(power_state) != 0U)
      {
        s_pending_start_request = 0U;
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_WAIT_CODECHIP);
      }
      else
      {
        s_pending_start_request = 1U;
      }
      break;

    case SEQUENCEMANAGER_CMD_CODECHIP_READY:
      if (s_state == SEQUENCEMANAGER_STATE_WAIT_CODECHIP)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_WAIT_CASSETTE);
      }
      break;

    case SEQUENCEMANAGER_CMD_CASSETTE_READY:
      if (s_state == SEQUENCEMANAGER_STATE_WAIT_CASSETTE)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_MEASURING);
      }
      break;

    case SEQUENCEMANAGER_CMD_MEASUREMENT_DONE:
      if (s_state == SEQUENCEMANAGER_STATE_MEASURING)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_CALCULATING);
      }
      break;

    case SEQUENCEMANAGER_CMD_CALCULATION_DONE:
      if (s_state == SEQUENCEMANAGER_STATE_CALCULATING)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_RESULT_DISPLAY);
      }
      break;

    case SEQUENCEMANAGER_CMD_RESET:
      s_pending_start_request = 0U;
      SequenceManager_SetState(SEQUENCEMANAGER_STATE_IDLE);
      break;

    case SEQUENCEMANAGER_CMD_NONE:
    default:
      break;
  }
}

/**
 * @brief SequenceManager 내부 상태와 명령 큐를 초기화한다.
 * @retval 0 초기화 성공.
 */
int32_t SequenceManager_App_Init(void)
{
  SequenceManager_CmdQueue_Init();
  s_pending_start_request = 0U;
  s_state = SEQUENCEMANAGER_STATE_IDLE;
  s_state_enter_tick = HAL_GetTick();

  return 0;
}

/**
 * @brief 명령 큐를 처리하고 pending 시작 요청 반영 여부를 검사한다.
 * @retval 0 실행 성공.
 */
int32_t SequenceManager_App_Run(void)
{
  SequenceManager_Command_t cmd;

  while (SequenceManager_CmdQueue_Pop(&cmd) == 0)
  {
    SequenceManager_HandleCommand(cmd);
  }

  if ((s_pending_start_request != 0U) &&
      (s_state == SEQUENCEMANAGER_STATE_IDLE) &&
      (SequenceManager_IsDisplayCapableState(PowerManager_Interface_GetState()) != 0U))
  {
    s_pending_start_request = 0U;
    SequenceManager_SetState(SEQUENCEMANAGER_STATE_WAIT_CODECHIP);
  }

  return 0;
}

/**
 * @brief 외부에서 전달한 명령을 내부 큐에 등록한다.
 * @param cmd 등록할 SequenceManager 명령.
 * @retval 0 등록 성공.
 * @retval -1 큐가 가득 차서 등록 실패.
 */
int32_t SequenceManager_App_SubmitCommand(SequenceManager_Command_t cmd)
{
  return SequenceManager_CmdQueue_Push(cmd);
}

/**
 * @brief 현재 검사 절차 상태를 반환한다.
 * @retval 현재 SequenceManager 상태.
 */
SequenceManager_State_t SequenceManager_App_GetState(void)
{
  return s_state;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
/* Sequence manager app (internal logic) */

#include "SequenceManager_App.h"

#include "App/CodeChip_App/CodeChip_LotValidator.h"
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

static SequenceManager_State_t s_state;          // 현재 검사절차 상태
static uint32_t s_state_enter_tick;              // 현재 상태에 진입한 시각 (ms 단위)
static uint8_t s_pending_start_request;          // 전원 복귀 후 반영할 시작 요청 보관 플래그
static uint8_t s_lot_validation_done;            // VALIDATE_LOT 상태에서 Stub 결과 1회 처리 플래그
static SequenceManager_CmdQueue_t s_cmd_queue;   // SequenceManager 명령 큐 인스턴스

static void SequenceManager_CmdQueue_Init(void);
static int32_t SequenceManager_CmdQueue_Push(SequenceManager_Command_t cmd);
static int32_t SequenceManager_CmdQueue_Pop(SequenceManager_Command_t* out_cmd);
static void SequenceManager_SetState(SequenceManager_State_t next_state);
static uint8_t SequenceManager_IsDisplayCapableState(PowerManager_State_t power_state);
static void SequenceManager_HandleCommand(SequenceManager_Command_t cmd);
static int32_t SequenceManager_RunLotValidation(void);

/**
 * @brief 내부 명령 큐 인덱스와 개수를 초기화한다.
 */
static void SequenceManager_CmdQueue_Init(void)
{
  // 1) 읽기 위치, 쓰기 위치, 저장 개수를 0으로 초기화
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
  // 1) 큐가 가득 찼으면 저장하지 않고 실패 반환
  if (s_cmd_queue.count >= SEQUENCEMANAGER_CMD_QUEUE_SIZE)
  {
    return -1;
  }

  // 2) 현재 tail 위치에 명령 저장
  s_cmd_queue.buffer[s_cmd_queue.tail] = cmd;
  // 3) tail 인덱스를 순환 방식으로 한 칸 전진
  s_cmd_queue.tail = (s_cmd_queue.tail + 1U) % SEQUENCEMANAGER_CMD_QUEUE_SIZE;
  // 4) 저장된 명령 개수를 1 증가
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
  // 1) 출력 포인터가 NULL이거나 큐가 비어 있으면 실패 반환
  if ((out_cmd == NULL) || (s_cmd_queue.count == 0U))
  {
    return -1;
  }

  // 2) 현재 head 위치의 명령을 출력 포인터에 복사
  *out_cmd = s_cmd_queue.buffer[s_cmd_queue.head];
  // 3) head 인덱스를 순환 방식으로 한 칸 전진
  s_cmd_queue.head = (s_cmd_queue.head + 1U) % SEQUENCEMANAGER_CMD_QUEUE_SIZE;
  // 4) 저장된 명령 개수를 1 감소
  s_cmd_queue.count--;

  return 0;
}

/**
 * @brief 현재 검사 절차 상태를 변경하고 진입 시각을 기록한다.
 * @param next_state 전이할 다음 상태.
 */
static void SequenceManager_SetState(SequenceManager_State_t next_state)
{
  // 1) 현재 상태를 새 상태로 교체
  s_state = next_state;
  // 2) 새 상태에 진입한 시각을 기록
  s_state_enter_tick = HAL_GetTick();

  // 3) VALIDATE_LOT에 새로 진입할 때만 Stub 검증 1회 처리 플래그를 리셋
  if (next_state == SEQUENCEMANAGER_STATE_VALIDATE_LOT)
  {
    s_lot_validation_done = 0U;
  }
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
 * @brief VALIDATE_LOT 상태에서 LotValidator Stub를 1회 실행하고 결과 명령을 큐에 등록한다.
 * @retval 0 처리 성공 또는 아직 처리하지 않음.
 * @retval -1 결과 명령 큐 등록 실패.
 */
static int32_t SequenceManager_RunLotValidation(void)
{
  CodeChip_LotValidator_Result_t result;
  SequenceManager_Command_t next_cmd;

  if ((s_state != SEQUENCEMANAGER_STATE_VALIDATE_LOT) ||
      (s_lot_validation_done != 0U))
  {
    return 0;
  }

  /* TODO: 실제 CodeChip read/parse 도입 시 s_state_enter_tick 기반 timeout 추가 검토 */
  result = CodeChip_LotValidator_Validate();

  switch (result)
  {
    case CODECHIP_LOT_VALIDATOR_RESULT_VALID:
      next_cmd = SEQUENCEMANAGER_CMD_LOT_VALID;
      break;

    case CODECHIP_LOT_VALIDATOR_RESULT_INVALID:
      next_cmd = SEQUENCEMANAGER_CMD_LOT_INVALID;
      break;

    case CODECHIP_LOT_VALIDATOR_RESULT_READ_FAIL:
    default:
      next_cmd = SEQUENCEMANAGER_CMD_LOT_READ_FAIL;
      break;
  }

  if (SequenceManager_CmdQueue_Push(next_cmd) != 0)
  {
    return -1;
  }

  s_lot_validation_done = 1U;
  return 0;
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

  // 1) 현재 전원 상태 조회
  power_state = PowerManager_Interface_GetState();

  // 2) 수신 명령 종류에 따라 상태 전이 처리
  switch (cmd)
  {
    case SEQUENCEMANAGER_CMD_START_REQUEST:
      // IDLE 상태가 아니면 시작 요청 무시
      if (s_state != SEQUENCEMANAGER_STATE_IDLE)
      {
        break;
      }
      // 표시 가능 전원 상태이면 WAIT_CODECHIP으로 즉시 전이, 아니면 pending 보관
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

    case SEQUENCEMANAGER_CMD_CODECHIP_INSERTED:
      // WAIT_CODECHIP 상태이면 VALIDATE_LOT로 전이
      if (s_state == SEQUENCEMANAGER_STATE_WAIT_CODECHIP)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_VALIDATE_LOT);
      }
      break;

    case SEQUENCEMANAGER_CMD_LOT_VALID:
      // VALIDATE_LOT 상태이면 WAIT_CASSETTE로 전이
      if (s_state == SEQUENCEMANAGER_STATE_VALIDATE_LOT)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_WAIT_CASSETTE);
      }
      break;

    case SEQUENCEMANAGER_CMD_LOT_INVALID:
    case SEQUENCEMANAGER_CMD_LOT_READ_FAIL:
      // VALIDATE_LOT 상태이면 ERROR로 전이
      if (s_state == SEQUENCEMANAGER_STATE_VALIDATE_LOT)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_ERROR);
      }
      break;

    case SEQUENCEMANAGER_CMD_CASSETTE_INSERTED:
      // WAIT_CASSETTE 상태이면 READY_TO_INCUBATE로 전이
      if (s_state == SEQUENCEMANAGER_STATE_WAIT_CASSETTE)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_READY_TO_INCUBATE);
      }
      break;

    case SEQUENCEMANAGER_CMD_MEASUREMENT_DONE:
      // MEASURING 상태이면 CALCULATING으로 전이
      if (s_state == SEQUENCEMANAGER_STATE_MEASURING)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_CALCULATING);
      }
      break;

    case SEQUENCEMANAGER_CMD_CALCULATION_DONE:
      // CALCULATING 상태이면 RESULT_DISPLAY로 전이
      if (s_state == SEQUENCEMANAGER_STATE_CALCULATING)
      {
        SequenceManager_SetState(SEQUENCEMANAGER_STATE_RESULT_DISPLAY);
      }
      break;

    case SEQUENCEMANAGER_CMD_RESET:
      // pending 플래그를 해제하고 IDLE로 복귀
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
  // 1) 명령 큐 인덱스와 개수 초기화
  SequenceManager_CmdQueue_Init();
  // 2) pending 시작 요청 플래그 초기화
  s_pending_start_request = 0U;
  // 3) Lot validation 1회 처리 플래그 초기화
  s_lot_validation_done = 0U;
  // 4) 초기 상태를 IDLE로 설정
  s_state = SEQUENCEMANAGER_STATE_IDLE;
  // 5) 상태 진입 시각 기록
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

  // 1) 큐에 쌓인 명령을 모두 꺼내 처리
  while (SequenceManager_CmdQueue_Pop(&cmd) == 0)
  {
    SequenceManager_HandleCommand(cmd);
  }

  // 2) pending 시작 요청이 있고 IDLE 상태이며 전원이 복귀했는지 확인
  if ((s_pending_start_request != 0U) &&
      (s_state == SEQUENCEMANAGER_STATE_IDLE) &&
      (SequenceManager_IsDisplayCapableState(PowerManager_Interface_GetState()) != 0U))
  {
    // 3) pending 플래그 해제 후 WAIT_CODECHIP으로 전이
    s_pending_start_request = 0U;
    SequenceManager_SetState(SEQUENCEMANAGER_STATE_WAIT_CODECHIP);
  }

  // 4) VALIDATE_LOT 상태에서는 Stub 결과를 1회만 평가해 다음 명령으로 변환
  (void)SequenceManager_RunLotValidation();

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
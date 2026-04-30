/* Screen manager app (internal logic) */

#include "ScreenManager_App.h"

#include "ST7735S_Drv.h"
#include "App/PowerManager_App/PowerManager_Interface.h"
#include "App/SequenceManager_App/SequenceManager_Interface.h"

/* [LEGACY] RGB565 색상 상수. Phase 5 이후 mono on/off API만 사용하므로 실제 참조되지 않음. */
#define SCREEN_BG_COLOR              0x0000U
#define SCREEN_TEXT_COLOR            0xFFFFU

/* Phase 6: mono dot 좌표 기준.
 *   기존 logical 좌표 (2, 2) → mono (2*3, 2) = (6, 2).
 *   기존 logical 좌표 (2, 12) → mono (6, 12).
 *   mono_x 범위: 0..255, mono_y 범위: 0..95. */
#define SCREEN_TEXT_X                6U    /* 기존 logical 2 → mono 6 */
#define SCREEN_TEXT_Y                2U    /* y는 1:1 */
#define SCREEN_BAT_LOW_Y             12U   /* y는 1:1 */
#define SCREEN_BOOT_HOLD_MS          600U
#define SCREEN_CMD_QUEUE_SIZE        8U

typedef struct
{
  ScreenManager_Command_t buffer[SCREEN_CMD_QUEUE_SIZE];
  uint32_t head;
  uint32_t tail;
  uint32_t count;
} ScreenManager_CmdQueue_t;

static ScreenManager_State_t s_state;
static uint32_t s_state_enter_tick;
static uint8_t s_need_redraw;
static ScreenManager_CmdQueue_t s_cmd_queue;

static void ScreenManager_CmdQueue_Init(void);
static int32_t ScreenManager_CmdQueue_Push(ScreenManager_Command_t cmd);
static int32_t ScreenManager_CmdQueue_Pop(ScreenManager_Command_t* out_cmd);
static void ScreenManager_SetState(ScreenManager_State_t next_state);
static void ScreenManager_HandleCommand(ScreenManager_Command_t cmd);
static uint8_t ScreenManager_IsDisplayScene(ScreenManager_State_t state);
static ScreenManager_State_t ScreenManager_ResolveState(PowerManager_State_t pm_state,
                                                        SequenceManager_State_t seq_state);
static void ScreenManager_RenderIfNeeded(void);

/**
  * @brief  화면 명령 큐의 인덱스와 개수를 초기화합니다.
  * @param  None
  * @retval None
  */
static void ScreenManager_CmdQueue_Init(void)
{
  // 1) 큐 시작 상태를 비워진 상태로 맞춥니다.
  s_cmd_queue.head = 0U;
  s_cmd_queue.tail = 0U;
  s_cmd_queue.count = 0U;
}

/**
  * @brief  화면 전환 명령을 큐에 저장합니다.
  * @param  cmd 저장할 화면 명령
  * @retval 0  저장 성공
  * @retval -1 큐가 가득 찬 경우
  */
static int32_t ScreenManager_CmdQueue_Push(ScreenManager_Command_t cmd)
{
  // 1) 큐가 가득 찼으면 더 이상 저장하지 않습니다.
  if (s_cmd_queue.count >= SCREEN_CMD_QUEUE_SIZE)
  {
    return -1;
  }

  // 2) 현재 tail 위치에 명령을 저장합니다.
  s_cmd_queue.buffer[s_cmd_queue.tail] = cmd;

  // 3) tail과 개수를 갱신해 다음 저장 위치를 준비합니다.
  s_cmd_queue.tail = (s_cmd_queue.tail + 1U) % SCREEN_CMD_QUEUE_SIZE;
  s_cmd_queue.count++;
  return 0;
}

/**
  * @brief  화면 명령 큐에서 다음 명령을 하나 꺼냅니다.
  * @param  out_cmd 꺼낸 명령을 저장할 출력 포인터
  * @retval 0  명령 읽기 성공
  * @retval -1 출력 포인터가 NULL이거나 큐가 비어 있는 경우
  */
static int32_t ScreenManager_CmdQueue_Pop(ScreenManager_Command_t* out_cmd)
{
  // 1) 출력 버퍼가 없거나 큐가 비어 있으면 읽지 않습니다.
  if ((out_cmd == NULL) || (s_cmd_queue.count == 0U))
  {
    return -1;
  }

  // 2) head 위치의 명령을 호출자에게 전달합니다.
  *out_cmd = s_cmd_queue.buffer[s_cmd_queue.head];

  // 3) head와 개수를 갱신해 다음 읽기 위치를 준비합니다.
  s_cmd_queue.head = (s_cmd_queue.head + 1U) % SCREEN_CMD_QUEUE_SIZE;
  s_cmd_queue.count--;
  return 0;
}

/**
  * @brief  화면 상태를 변경하고 redraw 요청을 설정합니다.
  * @param  next_state 전이할 다음 화면 상태
  * @retval None
  */
static void ScreenManager_SetState(ScreenManager_State_t next_state)
{
  // 1) 현재 화면 상태를 새 상태로 바꿉니다.
  s_state = next_state;

  // 2) 상태 진입 시각을 기록해 시간 기반 정책에 사용합니다.
  s_state_enter_tick = HAL_GetTick();

  // 3) 다음 Run에서 화면을 다시 그리도록 redraw를 요청합니다.
  s_need_redraw = 1U;
}

/**
  * @brief  수신한 화면 명령에 따라 화면 상태를 전환합니다.
  * @param  cmd 처리할 화면 명령
  * @retval None
  */
static void ScreenManager_HandleCommand(ScreenManager_Command_t cmd)
{
  // 1) 외부 명령이 Standby 요청이면 Standby 화면 상태로 전환합니다.
  if (cmd == SCREENMANAGER_CMD_SHOW_STANDBY)
  {
    ScreenManager_SetState(SCREENMANAGER_STATE_STANDBY);
  }
  // 2) Sleep 요청이면 Sleep 화면 상태로 전환합니다.
  else if (cmd == SCREENMANAGER_CMD_SHOW_SLEEP)
  {
    ScreenManager_SetState(SCREENMANAGER_STATE_SLEEP);
  }
  // 3) Power-off notice 요청이면 종료 안내 화면 상태로 전환합니다.
  else if (cmd == SCREENMANAGER_CMD_SHOW_POWER_OFF_NOTICE)
  {
    ScreenManager_SetState(SCREENMANAGER_STATE_POWER_OFF_NOTICE);
  }
  // 4) 그 외 명령은 이 단계에서 처리하지 않습니다.
  else
  {
    /* no-op */
  }
}

static uint8_t ScreenManager_IsDisplayScene(ScreenManager_State_t state)
{
  // 1) 텍스트나 overlay를 표시할 수 있는 장면만 display scene으로 봅니다.
  switch (state)
  {
    case SCREENMANAGER_STATE_STANDBY:
    case SCREENMANAGER_STATE_WAIT_CODECHIP:
    case SCREENMANAGER_STATE_VALIDATE_LOT:
    case SCREENMANAGER_STATE_WAIT_CASSETTE:
    case SCREENMANAGER_STATE_READY_TO_INCUBATE:
    case SCREENMANAGER_STATE_ERROR:
    case SCREENMANAGER_STATE_MEASURING:
    case SCREENMANAGER_STATE_CALCULATING:
    case SCREENMANAGER_STATE_RESULT_DISPLAY:
      return 1U;

    case SCREENMANAGER_STATE_BOOT:
    case SCREENMANAGER_STATE_SLEEP:
    case SCREENMANAGER_STATE_BLANK:
    case SCREENMANAGER_STATE_POWER_OFF_NOTICE:
    default:
      return 0U;
  }
}

static ScreenManager_State_t ScreenManager_ResolveState(PowerManager_State_t pm_state,
                                                        SequenceManager_State_t seq_state)
{
  // 1) Sleep 상태는 검사 상태보다 우선해서 Sleep 장면으로 고정합니다.
  if (pm_state == POWERMANAGER_STATE_SLEEP)
  {
    return SCREENMANAGER_STATE_SLEEP;
  }

  // 2) 전원 차단 안내 상태도 검사 상태보다 우선합니다.
  if (pm_state == POWERMANAGER_STATE_POWER_OFF_NOTICE)
  {
    return SCREENMANAGER_STATE_POWER_OFF_NOTICE;
  }

  // 3) 실제 전원 차단 상태에서는 blank 장면으로 보냅니다.
  if (pm_state == POWERMANAGER_STATE_POWER_OFF)
  {
    return SCREENMANAGER_STATE_BLANK;
  }

  // 4) 전원 쪽에서 강제할 장면이 없으면 검사 상태를 화면 상태로 변환합니다.
  switch (seq_state)
  {
    case SEQUENCEMANAGER_STATE_WAIT_CODECHIP:
      return SCREENMANAGER_STATE_WAIT_CODECHIP;

    case SEQUENCEMANAGER_STATE_VALIDATE_LOT:
      return SCREENMANAGER_STATE_VALIDATE_LOT;

    case SEQUENCEMANAGER_STATE_WAIT_CASSETTE:
      return SCREENMANAGER_STATE_WAIT_CASSETTE;

    case SEQUENCEMANAGER_STATE_READY_TO_INCUBATE:
      return SCREENMANAGER_STATE_READY_TO_INCUBATE;

    case SEQUENCEMANAGER_STATE_ERROR:
      return SCREENMANAGER_STATE_ERROR;

    case SEQUENCEMANAGER_STATE_MEASURING:
      return SCREENMANAGER_STATE_MEASURING;

    case SEQUENCEMANAGER_STATE_CALCULATING:
      return SCREENMANAGER_STATE_CALCULATING;

    case SEQUENCEMANAGER_STATE_RESULT_DISPLAY:
      return SCREENMANAGER_STATE_RESULT_DISPLAY;

    case SEQUENCEMANAGER_STATE_IDLE:
    default:
      return SCREENMANAGER_STATE_STANDBY;
  }
}

/**
  * @brief  redraw 요청이 있을 때 현재 상태에 맞는 화면을 다시 그립니다.
  * @param  None
  * @retval None
  */
static void ScreenManager_RenderIfNeeded(void)
{
  PowerManager_Context_t pm_ctx;

  // 1) redraw 요청이 없으면 이번 주기에는 아무 것도 그리지 않습니다.
  if (s_need_redraw == 0U)
  {
    return;
  }

  // 2) 새 장면을 그리기 전에 mono framebuffer를 OFF로 지웁니다 (Phase 6 표준 흐름).
  //    즉시 flush 하지 않고, 함수 끝의 FlushMono()에서 1회만 송신하여 깜박임 방지.
  ST7735S_Drv_ClearMonoBuffer(0U);

  // 3) 현재 화면 상태에 맞는 기본 문자열을 출력합니다.
  if (s_state == SCREENMANAGER_STATE_BOOT)
  {
#if 0 /* PHASE4_TEST: 빌드 후 육안 확인하면 #if 0 으로 바꾸거나 블록 통째로 삭제 */
    /* ===== Phase 4 검증 임시 코드 (1순위: 십자선 두께 비교) ===========
     *  목적: 가로 1 dot vs 세로 1 dot 굵기가 같은지 확인.
     *        세로선과 가로선 굵기가 같으면 → mono 1 dot 정확히 출력 중.
     *        가로선만 3배 두꺼우면 → packing 미적용 (불합격).
     *  표준 흐름: ClearMonoBuffer → DrawMonoDot... → FlushMono.
     *  관찰 후 BOOT→STANDBY 전환을 막기 위해 while(1)로 정지.
     * ============================================================= */
    {
      uint16_t i;

      ST7735S_Drv_ClearMonoBuffer(0U);

      // 1) 세로선: x=128 고정, y=10..85 (76 dot 두께 1)
      for (i = 10U; i < 86U; i++)
      {
        ST7735S_Drv_DrawMonoDot(128U, i, 1U);
      }
      // 2) 가로선: y=48 고정, x=10..245 (236 dot 두께 1)
      for (i = 10U; i < 246U; i++)
      {
        ST7735S_Drv_DrawMonoDot(i, 48U, 1U);
      }
      // 3) 모서리 4점: VIEW 범위 정확성 검증.
      ST7735S_Drv_DrawMonoDot(0U,   0U,  1U);
      ST7735S_Drv_DrawMonoDot(255U, 0U,  1U);
      ST7735S_Drv_DrawMonoDot(0U,   95U, 1U);
      ST7735S_Drv_DrawMonoDot(255U, 95U, 1U);

      // 4) 한 번에 송신.
      ST7735S_Drv_FlushMono();

      // 5) 다른 화면이 덮어쓰지 않도록 정지. (전원 차단 후 USB 재연결로 정상 복귀)
      while (1)
      {
        /* 관찰 대기 */
      }
    }
#else
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "INCLIX BOOT", 1U);
#endif
  }
  else if (s_state == SCREENMANAGER_STATE_STANDBY)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "INCLIX STANDBY", 1U);
  }
  else if (s_state == SCREENMANAGER_STATE_WAIT_CODECHIP)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "INSERT CODECHIP", 1U);
  }
  else if (s_state == SCREENMANAGER_STATE_VALIDATE_LOT)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "CHECKING LOT", 1U);
  }
  else if (s_state == SCREENMANAGER_STATE_WAIT_CASSETTE)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "INSERT CASSETTE", 1U);
  }
  else if (s_state == SCREENMANAGER_STATE_READY_TO_INCUBATE)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "PRESS START", 1U);
  }
  else if (s_state == SCREENMANAGER_STATE_ERROR)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "LOT ERROR", 1U);
  }
  else if (s_state == SCREENMANAGER_STATE_MEASURING)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "MEASURING...", 1U);
  }
  else if (s_state == SCREENMANAGER_STATE_CALCULATING)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "CALCULATING...", 1U);
  }
  else if (s_state == SCREENMANAGER_STATE_RESULT_DISPLAY)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "RESULT READY", 1U);
  }
  else if (s_state == SCREENMANAGER_STATE_SLEEP)
  {
    /* 화면 소거만 — 텍스트 없음 (Sleep 상태) */
  }
  else if (s_state == SCREENMANAGER_STATE_BLANK)
  {
    /* 화면 소거만 — 텍스트 없음 (Power off 이후 blank 상태) */
  }
  else if (s_state == SCREENMANAGER_STATE_POWER_OFF_NOTICE)
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_TEXT_Y, "TURNING OFF...", 1U);
  }
  else
  {
    /* no-op */
  }

  // 4) display scene에서는 battery low overlay를 추가로 출력할 수 있습니다.
  if ((ScreenManager_IsDisplayScene(s_state) != 0U) &&
      (PowerManager_Interface_GetContext(&pm_ctx) == 0) &&
      (pm_ctx.battery_low_latched != 0U))
  {
    ST7735S_Drv_DrawString3x5(SCREEN_TEXT_X, SCREEN_BAT_LOW_Y, "BAT LOW", 1U);
  }

  // 5) 구성된 framebuffer를 LCD로 1회에 송신합니다 (Phase 6 표준 흐름의 마지막 단계).
  ST7735S_Drv_FlushMono();

  // 6) redraw를 끝냈으므로 요청 플래그를 내립니다.
  s_need_redraw = 0U;
}

/**
  * @brief  LCD 드라이버와 화면 매니저 상태를 초기화합니다.
  * @param  None
  * @retval 0 초기화 성공
  */
int32_t ScreenManager_App_Init(void)
{
  // 1) LCD 드라이버를 먼저 초기화합니다.
  ST7735S_Drv_Init();

  // 2) 화면 명령 큐를 비우고 초기 화면 상태를 BOOT로 맞춥니다.
  ScreenManager_CmdQueue_Init();
  s_state = SCREENMANAGER_STATE_BOOT;

  // 3) BOOT 진입 시각과 첫 redraw 요청을 설정합니다.
  s_state_enter_tick = HAL_GetTick();
  s_need_redraw = 1U;

  return 0;
}

/**
  * @brief  화면 명령 처리, 부팅 화면 타이머, 전원 상태 동기화, redraw를 수행합니다.
  * @param  None
  * @retval 0 실행 성공
  */
int32_t ScreenManager_App_Run(void)
{
  ScreenManager_Command_t cmd;
  ScreenManager_State_t next_state;
  PowerManager_State_t pm_state;
  SequenceManager_State_t seq_state;

  // 1) 큐에 쌓인 화면 전환 명령을 모두 꺼내 현재 상태에 반영합니다.
  // TODO: 이 while문은 자칫 한 곳에 오래 머무를 수 있으므로, 재설계 시 큐에서 한 번에 하나씩 처리하는 방식을 검토해야함.
  while (ScreenManager_CmdQueue_Pop(&cmd) == 0)
  {
    ScreenManager_HandleCommand(cmd);
  }

  // 2) 부팅 화면 유지 시간이 경과하면 Standby 화면으로 자동 전환합니다.
  if ((s_state == SCREENMANAGER_STATE_BOOT) &&
      ((HAL_GetTick() - s_state_enter_tick) >= SCREEN_BOOT_HOLD_MS))
  {
    pm_state = PowerManager_Interface_GetState();
    seq_state = SequenceManager_Interface_GetState();
    ScreenManager_SetState(ScreenManager_ResolveState(pm_state, seq_state));
  }

  // 3) BOOT 이후에는 전원축과 검사축을 조합해 최종 장면을 재결정합니다.
  if (s_state != SCREENMANAGER_STATE_BOOT)
  {
    pm_state = PowerManager_Interface_GetState();
    seq_state = SequenceManager_Interface_GetState();
    next_state = ScreenManager_ResolveState(pm_state, seq_state);

    if (next_state != s_state)
    {
      ScreenManager_SetState(next_state);
    }
  }

  // 4) redraw 요청이 있는 경우에만 현재 상태에 맞는 화면을 다시 그립니다.
  ScreenManager_RenderIfNeeded();

  return 0;
}

/**
  * @brief  외부에서 요청한 화면 전환 명령을 큐에 등록합니다.
  * @param  cmd 등록할 화면 명령
  * @retval 0  등록 성공
  * @retval -1 큐가 가득 찬 경우
  */
int32_t ScreenManager_App_SubmitCommand(ScreenManager_Command_t cmd)
{
  // 1) 외부 요청 명령을 내부 큐에 저장합니다.
  return ScreenManager_CmdQueue_Push(cmd);
}

/**
  * @brief  현재 화면 상태를 반환합니다.
  * @param  None
  * @retval 현재 ScreenManager 상태값
  */
ScreenManager_State_t ScreenManager_App_GetState(void)
{
  // 1) 현재 화면 상태 스냅샷을 그대로 반환합니다.
  return s_state;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/* Screen manager app (internal logic) */

#include "ScreenManager_App.h"

#include "ST7735S_Drv.h"

#define SCREEN_BG_COLOR              0x0000U
#define SCREEN_TEXT_COLOR            0xFFFFU
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
static void ScreenManager_RenderIfNeeded(void);

static void ScreenManager_CmdQueue_Init(void)
{
  s_cmd_queue.head = 0U;
  s_cmd_queue.tail = 0U;
  s_cmd_queue.count = 0U;
}

static int32_t ScreenManager_CmdQueue_Push(ScreenManager_Command_t cmd)
{
  if (s_cmd_queue.count >= SCREEN_CMD_QUEUE_SIZE)
  {
    return -1;
  }

  s_cmd_queue.buffer[s_cmd_queue.tail] = cmd;
  s_cmd_queue.tail = (s_cmd_queue.tail + 1U) % SCREEN_CMD_QUEUE_SIZE;
  s_cmd_queue.count++;
  return 0;
}

static int32_t ScreenManager_CmdQueue_Pop(ScreenManager_Command_t* out_cmd)
{
  if ((out_cmd == NULL) || (s_cmd_queue.count == 0U))
  {
    return -1;
  }

  *out_cmd = s_cmd_queue.buffer[s_cmd_queue.head];
  s_cmd_queue.head = (s_cmd_queue.head + 1U) % SCREEN_CMD_QUEUE_SIZE;
  s_cmd_queue.count--;
  return 0;
}

static void ScreenManager_SetState(ScreenManager_State_t next_state)
{
  s_state = next_state;
  s_state_enter_tick = HAL_GetTick();
  s_need_redraw = 1U;
}

static void ScreenManager_HandleCommand(ScreenManager_Command_t cmd)
{
  if (cmd == SCREENMANAGER_CMD_SHOW_IDLE)
  {
    ScreenManager_SetState(SCREENMANAGER_STATE_IDLE);
  }
  else if (cmd == SCREENMANAGER_CMD_SHOW_POWER_OFF_HINT)
  {
    ScreenManager_SetState(SCREENMANAGER_STATE_POWER_OFF_HINT);
  }
  else
  {
    /* no-op */
  }
}

static void ScreenManager_RenderIfNeeded(void)
{
  if (s_need_redraw == 0U)
  {
    return;
  }

  ST7735S_Drv_Clear(SCREEN_BG_COLOR);

  if (s_state == SCREENMANAGER_STATE_BOOT)
  {
    ST7735S_Drv_DrawString3x5(2U, 2U, "INCLIX BOOT", SCREEN_TEXT_COLOR, SCREEN_BG_COLOR);
  }
  else if (s_state == SCREENMANAGER_STATE_IDLE)
  {
    ST7735S_Drv_DrawString3x5(2U, 2U, "INCLIX READY", SCREEN_TEXT_COLOR, SCREEN_BG_COLOR);
  }
  else
  {
    ST7735S_Drv_DrawString3x5(2U, 2U, "POWER OFF REQ", SCREEN_TEXT_COLOR, SCREEN_BG_COLOR);
  }

  s_need_redraw = 0U;
}

int32_t ScreenManager_App_Init(void)
{
  ST7735S_Drv_Init();

  ScreenManager_CmdQueue_Init();
  s_state = SCREENMANAGER_STATE_BOOT;
  s_state_enter_tick = HAL_GetTick();
  s_need_redraw = 1U;

  return 0;
}

int32_t ScreenManager_App_Run(void)
{
  ScreenManager_Command_t cmd;

  while (ScreenManager_CmdQueue_Pop(&cmd) == 0)
  {
    ScreenManager_HandleCommand(cmd);
  }

  if ((s_state == SCREENMANAGER_STATE_BOOT) &&
      ((HAL_GetTick() - s_state_enter_tick) >= SCREEN_BOOT_HOLD_MS))
  {
    ScreenManager_SetState(SCREENMANAGER_STATE_IDLE);
  }

  ScreenManager_RenderIfNeeded();

  return 0;
}

int32_t ScreenManager_App_SubmitCommand(ScreenManager_Command_t cmd)
{
  return ScreenManager_CmdQueue_Push(cmd);
}

ScreenManager_State_t ScreenManager_App_GetState(void)
{
  return s_state;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

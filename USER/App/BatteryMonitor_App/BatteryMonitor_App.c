/* Battery monitor app (internal logic) */

#include "BatteryMonitor_App.h"
#include "string.h"

/* Event queue (ring buffer) */
static BatteryMonitor_AppEvent_t s_event_queue[BATTERYMONITOR_APP_EVENT_QUEUE_SIZE];
static uint32_t s_queue_head = 0U;
static uint32_t s_queue_tail = 0U;
static uint32_t s_queue_count = 0U;

/* Current battery level */
static uint32_t s_current_level = BATTERY_LEVEL_HIGH;

static void BatteryMonitor_App_Que_Init(void);
static int32_t BatteryMonitor_App_Que_Push(uint32_t level);

static void BatteryMonitor_App_Que_Init(void)
{
  s_queue_head = 0U;
  s_queue_tail = 0U;
  s_queue_count = 0U;
  memset(s_event_queue, 0, sizeof(s_event_queue));
}

static int32_t BatteryMonitor_App_Que_Push(uint32_t level)
{
  if (s_queue_count >= BATTERYMONITOR_APP_EVENT_QUEUE_SIZE)
  {
    return -1;  /* Queue full */
  }

  s_event_queue[s_queue_tail].level = level;
  s_queue_tail = (s_queue_tail + 1U) % BATTERYMONITOR_APP_EVENT_QUEUE_SIZE;
  s_queue_count++;

  return 0;
}

void BatteryMonitor_App_Init(void)
{
  BatteryMonitor_App_Que_Init();

  /* Initialize DAC_Manager */
  DAC_Manager_Init();

  /* Start with high level assumption */
  s_current_level = BATTERY_LEVEL_HIGH;
}

void BatteryMonitor_App_Task(void)
{
  uint8_t result[3];  /* Comparison results for 3 thresholds */
  uint32_t new_level;

  /* Test threshold 0 (CRITICAL: 2048) */
  DAC_Manager_Set_RefValue(BAT_THRESHOLD_CRITICAL);
  HAL_Delay(5);
  DAC_Manager_Get_CompareResult(&result[0]);

  /* Test threshold 1 (LOW: 2172) */
  DAC_Manager_Set_RefValue(BAT_THRESHOLD_LOW);
  HAL_Delay(5);
  DAC_Manager_Get_CompareResult(&result[1]);

  /* Test threshold 2 (MEDIUM: 2296) */
  DAC_Manager_Set_RefValue(BAT_THRESHOLD_MEDIUM);
  HAL_Delay(5);
  DAC_Manager_Get_CompareResult(&result[2]);

  /* Level determination logic:
   * result[i] = 1: BAT > threshold[i]
   * result[i] = 0: BAT <= threshold[i]
   */
  if (result[0] == 0)
  {
    /* BAT <= 2048 */
    new_level = BATTERY_LEVEL_CRITICAL;
  }
  else if (result[1] == 0)
  {
    /* 2048 < BAT <= 2172 */
    new_level = BATTERY_LEVEL_LOW;
  }
  else if (result[2] == 0)
  {
    /* 2172 < BAT <= 2296 */
    new_level = BATTERY_LEVEL_MEDIUM;
  }
  else
  {
    /* BAT > 2296 */
    new_level = BATTERY_LEVEL_HIGH;
  }

  /* Push event to queue if level changed */
  if (new_level != s_current_level)
  {
    s_current_level = new_level;
    BatteryMonitor_App_Que_Push(new_level);
  }
}

uint32_t BatteryMonitor_App_GetLevel(void)
{
  return s_current_level;
}

int32_t BatteryMonitor_App_GetEvent(BatteryMonitor_AppEvent_t* event)
{
  if (s_queue_count == 0U)
  {
    return -1;
  }

  *event = s_event_queue[s_queue_head];
  s_queue_head = (s_queue_head + 1U) % BATTERYMONITOR_APP_EVENT_QUEUE_SIZE;
  s_queue_count--;

  return 0;
}

uint32_t BatteryMonitor_App_GetEventCount(void)
{
  return s_queue_count;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

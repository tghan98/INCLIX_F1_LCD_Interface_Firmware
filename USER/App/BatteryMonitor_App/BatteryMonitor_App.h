/* Battery monitor app (internal logic) */

#ifndef BATTERYMONITOR_APP_H
#define BATTERYMONITOR_APP_H

#include "main.h"
#include "DAC_Manager.h"

/* Battery level definitions */
#define BATTERY_LEVEL_CRITICAL    0U  /* <= 2048 */
#define BATTERY_LEVEL_LOW         1U  /* 2048 < BAT <= 2172 */
#define BATTERY_LEVEL_MEDIUM      2U  /* 2172 < BAT <= 2296 */
#define BATTERY_LEVEL_HIGH        3U  /* > 2296 */

/* Voltage thresholds (DAC 12-bit values) */
#define BAT_THRESHOLD_CRITICAL    2048U
#define BAT_THRESHOLD_LOW         2172U
#define BAT_THRESHOLD_MEDIUM      2296U
#define BAT_THRESHOLD_HIGH        2420U  /* Optional: full threshold */

/* Event queue */
#define BATTERYMONITOR_APP_EVENT_QUEUE_SIZE    10U

/* Battery event type */
typedef struct {
  uint32_t level;  /* Battery level (0-3) */
} BatteryMonitor_AppEvent_t;

void BatteryMonitor_App_Init(void);
void BatteryMonitor_App_Task(void);
int32_t BatteryMonitor_App_GetEvent(BatteryMonitor_AppEvent_t* event);
uint32_t BatteryMonitor_App_GetEventCount(void);

#endif /* BATTERYMONITOR_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

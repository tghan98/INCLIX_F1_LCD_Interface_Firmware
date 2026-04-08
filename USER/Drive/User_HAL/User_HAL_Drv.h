/**
  ******************************************************************************
  * @file           : User_HAL_Drv.h
  * @brief          : INCLIX 보드 하드웨어 제어용 HAL 래퍼 헤더
  ******************************************************************************
  * @details
  * 상위 App/Drive 계층이 직접 GPIO 핀 이름이나 HAL 핸들을 알지 않도록
  * 전원, LCD, DAC, 공통 유틸 접근 함수를 선언한다.
  ******************************************************************************
  */

#ifndef __USER_HAL_DRV_H__
#define __USER_HAL_DRV_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Function prototypes -------------------------------------------------------*/
int32_t UserHAL_Config(void);
uint32_t BSP_TickTimer(uint32_t *p_tick_timer, uint32_t wait_tick_time);

/* Power / Input -------------------------------------------------------------*/
void HW_PW_OnOff(uint32_t on_off);
int32_t HW_ReadPin_PWSW_Status(void);
int32_t HW_ReadPin_UsbDetect_Status(void);

void HW_PW_LED_ONnOFF(uint32_t on_off);
void HW_PW_LED_Toggle(void);
int32_t HW_ReadPin_PW_LED_Status(void);

/* LCD control ---------------------------------------------------------------*/
void HW_LCD_Power_ONnOFF(uint32_t on_off);
void HW_LCD_Reset(uint32_t on_off);
void HW_LCD_CS_Select(uint32_t select);
void HW_LCD_DC_Set(uint32_t is_data);

/* HAL handle access ---------------------------------------------------------*/
int32_t HW_DAC_CTRL(uint32_t dac_12b_count);
void *Read_LCD_SPI_HalDrive(void);
void *Read_LCD_BL_Timer_HalDrive(void);
void *Read_DAC_HalDrive(void);
void *Read_COMP_HalDrive(void);

/* Utility -------------------------------------------------------------------*/
void UserHAL_SwDelay(uint32_t delay_ms);

#endif /* __USER_HAL_DRV_H__ */

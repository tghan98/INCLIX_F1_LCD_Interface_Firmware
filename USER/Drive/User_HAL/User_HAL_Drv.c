/**
  ******************************************************************************
  * @file           : User_HAL_Drv.c
  * @brief          : INCLIX 보드 하드웨어 제어용 HAL 래퍼 드라이버
  ******************************************************************************
  * @details
  * 보드 의존 GPIO/HAL 제어를 이 파일에 모아 상위 로직과 하드웨어를 분리한다.
  * 전원, LCD, DAC, 공통 HAL 핸들 접근을 한 곳에서 관리한다.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "User_HAL_Drv.h"

/* Private define ------------------------------------------------------------*/
#define POWER_STATUS_LED_GPIO_Port   GPIOC
#define POWER_STATUS_LED_Pin         GPIO_PIN_13

/* Extern --------------------------------------------------------------------*/
extern DAC_HandleTypeDef hdac1;
extern SPI_HandleTypeDef hspi2;
extern TIM_HandleTypeDef htim14;
extern COMP_HandleTypeDef hcomp2;

/* Private function prototypes -----------------------------------------------*/
static GPIO_PinState UserHAL_ToPinState(uint32_t on_off);

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  on/off 의미값을 GPIO 핀 상태값으로 변환한다.
  * @param  on_off: 0이면 RESET, 0이 아니면 SET
  * @retval GPIO_PIN_SET 또는 GPIO_PIN_RESET
  */
static GPIO_PinState UserHAL_ToPinState(uint32_t on_off)
{
  if (on_off != 0U)
  {
    return GPIO_PIN_SET;
  }

  return GPIO_PIN_RESET;
}

/* Function ------------------------------------------------------------------*/
/**
  * @brief  User HAL 초기화용 확장 함수이다.
  * @retval 0: 정상
  * @note   현재는 예약 함수이며, 필요 시 기본 출력 상태 설정에 사용한다.
  */
int32_t UserHAL_Config(void)
{
  return 0;
}

/**
  * @brief  간단한 비차단 tick 타이머 동작을 제공한다.
  * @param  p_tick_timer: 시작 tick 저장 변수 포인터
  * @param  wait_tick_time: 대기 시간 (tick)
  * @retval SET: 아직 대기 중
  * @retval RESET: 대기 시간이 경과함
  */
uint32_t BSP_TickTimer(uint32_t *p_tick_timer, uint32_t wait_tick_time)
{
  uint32_t current_tick;

  if (p_tick_timer == NULL)
  {
    return RESET;
  }

  current_tick = HAL_GetTick();

  if (*p_tick_timer == 0U)
  {
    *p_tick_timer = current_tick;
  }
  else if ((current_tick - *p_tick_timer) >= wait_tick_time)
  {
    *p_tick_timer = 0U;
    return RESET;
  }

  return SET;
}

/**
  * @brief  메인 전원 홀드 핀을 제어한다.
  * @param  on_off: 0이 아니면 ON, 0이면 OFF
  * @retval None
  */
void HW_PW_OnOff(uint32_t on_off)
{
  HAL_GPIO_WritePin(MPW_ONOFF_GPIO_Port, MPW_ONOFF_Pin, UserHAL_ToPinState(on_off));
}

/**
  * @brief  전원 스위치 입력 상태를 읽는다.
  * @retval SET: 입력 있음, RESET: 입력 없음
  */
int32_t HW_ReadPin_PWSW_Status(void)
{
  if (HAL_GPIO_ReadPin(PC_SW_SIG_GPIO_Port, PC_SW_SIG_Pin) == GPIO_PIN_SET)
  {
    return SET;
  }

  return RESET;
}

/**
  * @brief  외부 전원(USB) 감지 입력 상태를 읽는다.
  * @retval SET: 감지됨, RESET: 미감지
  */
int32_t HW_ReadPin_UsbDetect_Status(void)
{
  if (HAL_GPIO_ReadPin(EX_PW_CK_GPIO_Port, EX_PW_CK_Pin) == GPIO_PIN_SET)
  {
    return SET;
  }

  return RESET;
}

/**
  * @brief  전원 상태 LED를 켜거나 끈다.
  * @param  on_off: 0이 아니면 ON, 0이면 OFF
  * @retval None
  */
void HW_PW_LED_ONnOFF(uint32_t on_off)
{
  HAL_GPIO_WritePin(POWER_STATUS_LED_GPIO_Port, POWER_STATUS_LED_Pin, UserHAL_ToPinState(on_off));
}

/**
  * @brief  전원 상태 LED를 토글한다.
  * @retval None
  */
void HW_PW_LED_Toggle(void)
{
  HAL_GPIO_TogglePin(POWER_STATUS_LED_GPIO_Port, POWER_STATUS_LED_Pin);
}

/**
  * @brief  전원 상태 LED의 현재 핀 상태를 읽는다.
  * @retval SET: High, RESET: Low
  */
int32_t HW_ReadPin_PW_LED_Status(void)
{
  if (HAL_GPIO_ReadPin(POWER_STATUS_LED_GPIO_Port, POWER_STATUS_LED_Pin) == GPIO_PIN_SET)
  {
    return SET;
  }

  return RESET;
}

/**
  * @brief  LCD 전원 핀을 제어한다.
  * @param  on_off: 0이 아니면 ON, 0이면 OFF
  * @retval None
  */
void HW_LCD_Power_ONnOFF(uint32_t on_off)
{
  HAL_GPIO_WritePin(LCDVCC_EN_GPIO_Port, LCDVCC_EN_Pin, UserHAL_ToPinState(on_off));
}

/**
  * @brief  LCD 리셋 핀을 제어한다.
  * @param  on_off: 0이 아니면 High, 0이면 Low
  * @retval None
  */
void HW_LCD_Reset(uint32_t on_off)
{
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, UserHAL_ToPinState(on_off));
}

/**
  * @brief  LCD CS 핀을 제어한다. (active-low)
  * @param  select: 0이 아니면 선택, 0이면 해제
  * @retval None
  */
void HW_LCD_CS_Select(uint32_t select)
{
  if (select != 0U)
  {
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
  }
  else
  {
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
  }
}

/**
  * @brief  LCD DC 핀을 설정한다.
  * @param  is_data: 0이 아니면 데이터 모드, 0이면 명령 모드
  * @retval None
  */
void HW_LCD_DC_Set(uint32_t is_data)
{
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, UserHAL_ToPinState(is_data));
}

/**
  * @brief  DAC 채널 1의 출력 값을 설정한다.
  * @param  dac_12b_count: 12bit DAC 출력값
  * @retval HAL_DAC_SetValue() 반환값
  */
int32_t HW_DAC_CTRL(uint32_t dac_12b_count)
{
  return (int32_t)HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_12b_count);
}

/**
  * @brief  LCD 제어에 사용하는 SPI HAL 핸들을 반환한다.
  * @retval `SPI_HandleTypeDef *`
  */
void *Read_LCD_SPI_HalDrive(void)
{
  return (void *)&hspi2;
}

/**
  * @brief  LCD 백라이트 PWM용 타이머 핸들을 반환한다.
  * @retval `TIM_HandleTypeDef *`
  */
void *Read_LCD_BL_Timer_HalDrive(void)
{
  return (void *)&htim14;
}

/**
  * @brief  DAC HAL 핸들을 반환한다.
  * @retval `DAC_HandleTypeDef *`
  */
void *Read_DAC_HalDrive(void)
{
  return (void *)&hdac1;
}

/**
  * @brief  비교기 HAL 핸들을 반환한다.
  * @retval `COMP_HandleTypeDef *`
  */
void *Read_COMP_HalDrive(void)
{
  return (void *)&hcomp2;
}

/**
  * @brief  간단한 소프트웨어 지연 함수이다.
  * @param  delay_ms: 대기 시간 (ms)
  * @retval None
  */
void UserHAL_SwDelay(uint32_t delay_ms)
{
  HAL_Delay(delay_ms);
}

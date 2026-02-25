/**
  ******************************************************************************
  * @file           : Button_Drv.h
  * @brief          : 버튼 드라이버 헤더 (L1 - 하드웨어 추상화)
  *                   단일 버튼의 GPIO 제어, 디바운싱, 상태머신
  ******************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef BUTTON_DRV_H
#define BUTTON_DRV_H

/* Includes */
#include "main.h"

/* Defines */
/* 디바운스 시간 (밀리초) */
#define BUTTON_DEBOUNCE_MS    50U

/* Typedef */
/**
  * @brief 버튼 이벤트 타입
  * @note 상태머신이 한 사이클(Button_Drv_Update 호출)에서 반환할 이벤트
  */
typedef enum
{
  BUTTON_EVENT_NONE = 0,       /* 이벤트 없음 */
  BUTTON_EVENT_PRESSED,        /* 버튼 눌림 (디바운스 후) */
  BUTTON_EVENT_RELEASED,       /* 버튼 뗌 (디바운스 후) */
  BUTTON_EVENT_CLICK,          /* 짧은 클릭 완료 (눌렀다가 뗌) */
} ButtonEvent_t;

/**
  * @brief 버튼 내부 상태 (상태머신)
  */
typedef enum
{
  BUTTON_STATE_IDLE = 0,       /* 초기 상태 */
  BUTTON_STATE_DEBOUNCING,     /* 디바운싱 대기 중 */
  BUTTON_STATE_PRESSED,        /* 버튼이 눌린 상태 */
} ButtonState_t;

/**
  * @brief 버튼 객체 구조체 (상태 관리)
  * @note 각 물리 버튼마다 하나씩 생성됨
  */
typedef struct
{
  GPIO_TypeDef* port;          /* GPIO 포트 (GPIOA, GPIOB 등) */
  uint16_t pin;                /* GPIO 핀 (GPIO_PIN_0, GPIO_PIN_1 등) */
  GPIO_PinState active_level;  /* 버튼 눌림 레벨 (GPIO_PIN_RESET=Active Low, GPIO_PIN_SET=Active High) */
  
  ButtonState_t state;         /* 현재 상태머신 상태 */
  uint32_t last_change_tick;   /* 마지막으로 상태가 변경된 시간 (디바운싱용) */
} Button_t;

/* Function prototypes */

/**
  * @brief 버튼 드라이버 초기화
  * @param btn: 초기화할 버튼 객체 포인터
  * @param port: GPIO 포트 (예: GPIOB)
  * @param pin: GPIO 핀 (예: GPIO_PIN_0)
  * @param active_level: 버튼 눌림을 나타내는 PIN 레벨
  *                      Active Low: GPIO_PIN_RESET (버튼 누르면 0)
  *                      Active High: GPIO_PIN_SET (버튼 누르면 1)
  * @retval None
  */
void Button_Drv_Init(Button_t* btn, GPIO_TypeDef* port, uint16_t pin, GPIO_PinState active_level);

/**
  * @brief 버튼 업데이트 (상태머신 실행 및 이벤트 감지)
  * @note 이 함수는 주기적으로 호출되어야 함 (권장: 10~20ms 주기)
  * @param btn: 업데이트할 버튼 객체 포인터
  * @retval 발생한 이벤트 (ButtonEvent_t)
  *         - BUTTON_EVENT_NONE: 이벤트 없음
  *         - BUTTON_EVENT_PRESSED: 버튼 눌림 감지
  *         - BUTTON_EVENT_RELEASED: 버튼 뗌 감지
  *         - BUTTON_EVENT_CLICK: 클릭 완료 (눌렀다 뗌)
  */
ButtonEvent_t Button_Drv_Update(Button_t* btn);

#endif /* BUTTON_DRV_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

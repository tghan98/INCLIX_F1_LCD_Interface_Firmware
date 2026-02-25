/**
  ******************************************************************************
  * @file           : Button_App.h
  * @brief          : 버튼 앱 헤더 (L2 - 비즈니스 로직)
  *                   여러 버튼 관리, 이벤트 큐
  *                   NOTE: 이 파일은 PRIVATE입니다. 외부는 Button_Interface.h를 사용하세요.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef BUTTON_APP_H
#define BUTTON_APP_H

/* Includes */
#include "main.h"
#include "Button_Drv.h"
#include "Button_Interface.h"  /* L3의 public 타입을 가져옴 (ButtonId_t, ButtonEvent_t 등) */

/* Defines */
/* 이벤트 큐 크기 */
#define BUTTON_APP_EVENT_QUEUE_SIZE    10U

/* Typedef */

/* ButtonId_t는 Button_Interface.h에 정의됨 (재정의 안함) */
/* ButtonEvent_t는 Button_Drv.h에 정의됨 (재정의 안함) */
/* ButtonAppEvent_t는 Button_Interface.h에 정의됨 (재정의 안함) */

/* Function prototypes (PRIVATE - 외부에서 호출하면 안됨) */

/**
  * @brief 버튼 앱 초기화
  * @note Button_Interface_Init()에서만 호출됨
  * @retval 0 = 성공
  */
void Button_App_Init(void);

/**
  * @brief 버튼 앱 태스크 (모든 버튼 스캔 및 이벤트 생성)
  * @note Button_Interface_Run()에서만 호출됨
  * @retval 0 = 성공
  */
void Button_App_Task(void);

/**
  * @brief 이벤트 큐에서 이벤트 하나 가져오기
  * @note Button_Interface_GetEvent()에서만 호출됨
  * @param event: 받은 이벤트 저장 포인터
  * @retval 0 = 성공 (이벤트 받음)
  *        -1 = 큐가 비어있음
  */
int32_t Button_App_GetEvent(ButtonAppEvent_t* event);

/**
  * @brief 이벤트 큐에 남아있는 이벤트 개수 확인
  * @note 선택사항 함수
  * @retval 큐에 있는 이벤트 개수
  */
uint32_t Button_App_GetEventCount(void);

#endif /* BUTTON_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

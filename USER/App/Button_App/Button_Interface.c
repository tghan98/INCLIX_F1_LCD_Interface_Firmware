/**
  ******************************************************************************
  * @file           : Button_Interface.c
  * @brief          : 버튼 인터페이스 구현 (L3 - 공개 인터페이스)
  *                   Button_App의 private 함수들을 감싸서 공개 API 제공
  ******************************************************************************
  */

/* Includes */
#include "Button_Interface.h"
#include "Button_App.h"  /* private - Button_Interface.c에서만 include */

/* Private defines */

/* Private typedef */

/* Private variables */

/* Private function prototypes */

/* Function implementation */

/**
  * @brief 버튼 인터페이스 초기화
  * @note Button_App_Init()을 감싸서 호출
  * @retval 0 = 성공
  */
int32_t Button_Interface_Init(void)
{
  Button_App_Init();
  return 0;
}

/**
  * @brief 버튼 인터페이스 실행 (버튼 스캔 및 이벤트 생성)
  * @note Button_App_Task()를 감싸서 호출
  * @retval 0 = 성공
  */
int32_t Button_Interface_Run(void)
{
  Button_App_Task();
  return 0;
}

/**
  * @brief 이벤트 큐에서 이벤트 하나 가져오기
  * @note Button_App_GetEvent()를 감싸서 호출
  * @param event: 받은 이벤트 저장 포인터
  * @retval 0 = 성공 (이벤트 받음)
  *        -1 = 큐가 비어있음
  */
int32_t Button_GetEvent(ButtonAppEvent_t* event)
{
  return Button_App_GetEvent(event);
}

/**
  * @brief 이벤트 큐에 남아있는 이벤트 개수 확인 (선택사항)
  * @note Button_App_GetEventCount()를 감싸서 호출
  * @retval 큐에 있는 이벤트 개수
  */
uint32_t Button_GetEventCount(void)
{
  return Button_App_GetEventCount();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

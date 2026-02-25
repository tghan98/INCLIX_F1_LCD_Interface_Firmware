/**
  ******************************************************************************
  * @file           : Button_Interface.h
  * @brief          : 버튼 인터페이스 헤더 (L3 - 공개 인터페이스)
  *                   외부 모듈이 유일하게 include해야 할 파일
  *                   Button_App의 내부 구현을 숨기고 공개 API만 제공
  ******************************************************************************
  */

/* Define to prevent recursive inclusion */
#ifndef BUTTON_INTERFACE_H
#define BUTTON_INTERFACE_H

/* Includes */
#include "main.h"
#include "Button_Drv.h"  /* L1 드라이버의 타입을 가져옴 (ButtonEvent_t) */

/* Typedef */

/**
  * @brief 버튼 ID (시스템의 모든 버튼 식별자)
  */
typedef enum
{
  BUTTON_ID_PAUSE = 0,   /* 일시정지 버튼 */
  BUTTON_ID_MAX
} ButtonId_t;

/**
  * @brief 버튼 이벤트 메시지
  * @note ButtonEvent_t는 Button_Drv.h에서 정의됨
  */
typedef struct
{
  ButtonId_t button_id;      /* 어떤 버튼 */
  ButtonEvent_t event;       /* 어떤 이벤트 (Button_Drv.h의 타입) */
} ButtonAppEvent_t;

/* Function prototypes (PUBLIC - 외부에서 호출 가능) */

/**
  * @brief 버튼 인터페이스 초기화
  * @note User_Main_Init()에서 호출됨
  * @retval 0 = 성공
  */
int32_t Button_Interface_Init(void);

/**
  * @brief 버튼 인터페이스 실행 (버튼 스캔 및 이벤트 생성)
  * @note User_Main_Run()에서 호출됨 (주기적으로)
  * @retval 0 = 성공
  */
int32_t Button_Interface_Run(void);

/**
  * @brief 이벤트 큐에서 이벤트 하나 가져오기
  * @note 다른 앱에서 호출하여 버튼 이벤트를 받음
  * @param event: 받은 이벤트 저장 포인터
  * @retval 0 = 성공 (이벤트 받음)
  *        -1 = 큐가 비어있음
  */
int32_t Button_GetEvent(ButtonAppEvent_t* event);

/**
  * @brief 이벤트 큐에 남아있는 이벤트 개수 확인 (선택사항)
  * @retval 큐에 있는 이벤트 개수
  */
uint32_t Button_GetEventCount(void);

#endif /* BUTTON_INTERFACE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

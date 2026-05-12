/* Sequence manager app (internal logic) */

#ifndef SEQUENCEMANAGER_APP_H
#define SEQUENCEMANAGER_APP_H

#include "main.h"
#include "App/AnalysisEngine_App/AnalysisEngine_Types.h"

/* 검사 절차 상태 정의 */
typedef enum
{
  SEQUENCEMANAGER_STATE_IDLE = 0,          // 대기 상태
  SEQUENCEMANAGER_STATE_WAIT_CODECHIP,     // 코드칩 삽입 대기
  SEQUENCEMANAGER_STATE_VALIDATE_LOT,      // Lot 검증 중
  SEQUENCEMANAGER_STATE_WAIT_CASSETTE,     // 카세트 삽입 대기
  SEQUENCEMANAGER_STATE_READY_TO_INCUBATE, // 인큐베이션 시작 준비 완료
  SEQUENCEMANAGER_STATE_INCUBATION,        // 인큐베이션 진행 중
  SEQUENCEMANAGER_STATE_ERROR,             // 분석 준비 중 오류
  SEQUENCEMANAGER_STATE_MEASURING,         // 측정 진행 중
  SEQUENCEMANAGER_STATE_CALCULATING,       // 측정 결과 계산 중
  SEQUENCEMANAGER_STATE_RESULT_DISPLAY     // 결과 표시 상태
} SequenceManager_State_t;

/* 검사 절차 명령 정의 */
typedef enum
{
  SEQUENCEMANAGER_CMD_NONE = 0,          // 유효한 명령 없음
  SEQUENCEMANAGER_CMD_START_REQUEST,     // 검사 시작 요청
  SEQUENCEMANAGER_CMD_INCUBATION_START,  // 인큐베이션 시작
  SEQUENCEMANAGER_CMD_CODECHIP_INSERTED, // 코드칩 삽입 완료
  SEQUENCEMANAGER_CMD_LOT_VALID,         // Lot 검증 성공
  SEQUENCEMANAGER_CMD_LOT_INVALID,       // Lot 검증 실패
  SEQUENCEMANAGER_CMD_LOT_READ_FAIL,     // Lot 읽기 실패
  SEQUENCEMANAGER_CMD_CASSETTE_INSERTED, // 카세트 삽입 완료
  SEQUENCEMANAGER_CMD_MEASURE_START,     // 측정 시작
  SEQUENCEMANAGER_CMD_MEASUREMENT_DONE,  // 측정 완료
  SEQUENCEMANAGER_CMD_CALCULATION_DONE,  // 계산 완료
  SEQUENCEMANAGER_CMD_RESET              // 절차 초기 상태로 복귀
} SequenceManager_Command_t;

/**
 * @brief SequenceManager 내부 상태와 큐를 초기화한다.
 * @retval 0 초기화 성공.
 */
int32_t SequenceManager_App_Init(void);

/**
 * @brief SequenceManager 상태머신을 한 주기 실행한다.
 * @retval 0 실행 성공.
 */
int32_t SequenceManager_App_Run(void);

/**
 * @brief 외부에서 전달한 절차 명령을 내부 큐에 등록한다.
 * @param cmd 등록할 SequenceManager 명령.
 * @retval 0 등록 성공.
 * @retval -1 등록 실패.
 */
int32_t SequenceManager_App_SubmitCommand(SequenceManager_Command_t cmd);

/**
 * @brief 현재 검사 절차 상태를 반환한다.
 * @retval 현재 SequenceManager 상태.
 */
SequenceManager_State_t SequenceManager_App_GetState(void);

/**
 * @brief 마지막 계산 결과를 반환한다.
 * @param out_result 결과 구조체의 const 포인터를 저장할 출력 포인터.
 * @retval 0 성공.
 * @retval -1 출력 포인터가 NULL.
 */
int32_t SequenceManager_App_GetLastResult(const AnalysisResult_t** out_result);

#endif /* SEQUENCEMANAGER_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
/* Sequence manager app (internal logic) */

#ifndef SEQUENCEMANAGER_APP_H
#define SEQUENCEMANAGER_APP_H

#include "main.h"

/* 검사 절차 상태 정의 */
typedef enum
{
  SEQUENCEMANAGER_STATE_IDLE = 0,       // 대기 상태
  SEQUENCEMANAGER_STATE_WAIT_CODECHIP,  // 코드칩 삽입 대기
  SEQUENCEMANAGER_STATE_VALIDATE_LOT,   // Lot 검증 중
  SEQUENCEMANAGER_STATE_WAIT_CASSETTE,  // 카세트 삽입 대기 
  SEQUENCEMANAGER_STATE_READY_TO_INCUBATE, // 인큐베이션 시작 준비 완료
  SEQUENCEMANAGER_STATE_INCUBATION,     // 인큐베이션 진행 중
  SEQUENCEMANAGER_STATE_ERROR,          // 분석 준비 중 오류
  SEQUENCEMANAGER_STATE_MEASURING,      // 측정 진행 중
  SEQUENCEMANAGER_STATE_CALCULATING,    // 측정 결과 계산 중
  SEQUENCEMANAGER_STATE_RESULT_DISPLAY  // 결과 표시 상태
} SequenceManager_State_t;

/* 검사 절차 명령 정의 */
typedef enum
{
  SEQUENCEMANAGER_CMD_NONE = 0,         // 유효한 명령 없음
  SEQUENCEMANAGER_CMD_START_REQUEST,    // 검사 시작 요청
  SEQUENCEMANAGER_CMD_INCUBATION_START, // 인큐베이션 시작
  SEQUENCEMANAGER_CMD_CODECHIP_INSERTED,// 코드칩 삽입 완료
  SEQUENCEMANAGER_CMD_LOT_VALID,        // Lot 검증 성공
  SEQUENCEMANAGER_CMD_LOT_INVALID,      // Lot 검증 실패
  SEQUENCEMANAGER_CMD_LOT_READ_FAIL,    // Lot 읽기 실패
  SEQUENCEMANAGER_CMD_CASSETTE_INSERTED,// 카세트 삽입 완료
  SEQUENCEMANAGER_CMD_MEASURE_START,    // 측정 시작
  SEQUENCEMANAGER_CMD_MEASUREMENT_DONE, // 측정 완료
  SEQUENCEMANAGER_CMD_CALCULATION_DONE, // 계산 완료
  SEQUENCEMANAGER_CMD_RESET             // 절차 초기 상태로 복귀
} SequenceManager_Command_t;

/**
 * @brief SequenceManager 내부 상태와 큐를 초기화한다.
 * @retval 0 초기화 성공.
 */
int32_t SequenceManager_App_Init(void);

/**
 * @brief SequenceManager 상태머신을 한 주기 실행한다.
 * @retval 0 실행 성공.
 */
int32_t SequenceManager_App_Run(void);

/**
 * @brief 외부에서 전달한 절차 명령을 내부 큐에 등록한다.
 * @param cmd 등록할 SequenceManager 명령.
 * @retval 0 등록 성공.
 * @retval -1 등록 실패.
 */
int32_t SequenceManager_App_SubmitCommand(SequenceManager_Command_t cmd);

/**
 * @brief 현재 검사 절차 상태를 반환한다.
 * @retval 현재 SequenceManager 상태.
 */
SequenceManager_State_t SequenceManager_App_GetState(void);

/**
 * @brief 마지막 계산 결과를 반환한다.
 * @param out_result 결과 구조체의 const 포인터를 저장할 출력 포인터.
 * @retval 0 성공.
 * @retval -1 출력 포인터가 NULL.
 */
int32_t SequenceManager_App_GetLastResult(const AnalysisResult_t** out_result);

#endif /* SEQUENCEMANAGER_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
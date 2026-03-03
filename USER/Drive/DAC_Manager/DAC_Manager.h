#ifndef DAC_MANAGER_H
#define DAC_MANAGER_H

#include "main.h"

//------------------------------------------------------------------------------
// 반환값 정의
#define DAC_MAN_SUCCESS         (0)
#define DAC_MAN_SYSTEM_ERR      (-1)
#define DAC_MAN_INVALID_PARAM   (-2)
#define DAC_MAN_BUSY            (-3)   // 이전 요청이 진행 중
#define DAC_MAN_NOT_READY       (-4)   // 아직 안정화 대기 중

//------------------------------------------------------------------------------
// 상태 enum (IDDD_ADC_Manager 모방)
typedef enum
{
  DAC_STATE_IDLE,        // 0: 대기 상태
  DAC_STATE_SETTING,     // 1: DAC 값 설정 + 5ms 안정화 중
  DAC_STATE_COMPLETE,    // 2: 비교 결과 읽을 준비 완료
} DAC_Manager_State_t;

//------------------------------------------------------------------------------
// 함수 인터페이스 (비블로킹 + 상태머신 구조)
void DAC_Manager_Init(void);

// Step 1: 비블로킹 시작
int32_t DAC_Manager_Start_Set_RefValue(uint16_t dac_value);

// Step 2: 준비 상태 확인 (5ms 경과 확인)
int32_t DAC_Manager_Check_Ready(void);

// Step 3: 비교 결과 읽기
int32_t DAC_Manager_Get_CompareResult(uint8_t *p_is_higher);

// 상태 조회
DAC_Manager_State_t DAC_Manager_Get_State(void);

#endif /* DAC_MANAGER_H */

#ifndef DAC_MANAGER_H
#define DAC_MANAGER_H

#include "main.h"

typedef enum
{
  DAC_MAN_SUCCESS = 0,
  DAC_MAN_SYSTEM_ERR = -1,
  DAC_MAN_INVALID_PARAM = -2
} DAC_Manager_Result_t;

int32_t DAC_Manager_Init(void);
int32_t DAC_Manager_Set_RefValue(uint16_t dac_value);
int32_t DAC_Manager_Get_CompareResult(uint8_t *p_is_higher);

#endif /* DAC_MANAGER_H */

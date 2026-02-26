#include "DAC_Manager.h"

extern DAC_HandleTypeDef hdac1;
extern COMP_HandleTypeDef hcomp2;

static uint8_t s_dac_manager_initialized = 0U;

int32_t DAC_Manager_Init(void)
{
  if (s_dac_manager_initialized != 0U)
  {
    return DAC_MAN_SUCCESS;
  }

  if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_1) != HAL_OK)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  if (HAL_COMP_Start(&hcomp2) != HAL_OK)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  s_dac_manager_initialized = 1U;
  return DAC_MAN_SUCCESS;
}

int32_t DAC_Manager_Set_RefValue(uint16_t dac_value)
{
  if (s_dac_manager_initialized == 0U)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  if (dac_value > 0x0FFFU)
  {
    return DAC_MAN_INVALID_PARAM;
  }

  if (HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value) != HAL_OK)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  return DAC_MAN_SUCCESS;
}

int32_t DAC_Manager_Get_CompareResult(uint8_t *p_is_higher)
{
  uint32_t comp_level;

  if (s_dac_manager_initialized == 0U || p_is_higher == NULL)
  {
    return DAC_MAN_SYSTEM_ERR;
  }

  comp_level = HAL_COMP_GetOutputLevel(&hcomp2);

  /* COMP output high means BAT_LV_ADC (InputPlus) > DAC1_CH1 (InputMinus). */
  *p_is_higher = (comp_level == COMP_OUTPUT_LEVEL_HIGH) ? 1U : 0U;

  return DAC_MAN_SUCCESS;
}

/* CodeChip lot validator stub (phase 1) */

#ifndef CODECHIP_LOTVALIDATOR_H
#define CODECHIP_LOTVALIDATOR_H

#include "main.h"

typedef enum
{
  CODECHIP_LOT_VALIDATOR_RESULT_VALID = 0,
  CODECHIP_LOT_VALIDATOR_RESULT_INVALID,
  CODECHIP_LOT_VALIDATOR_RESULT_READ_FAIL
} CodeChip_LotValidator_Result_t;

CodeChip_LotValidator_Result_t CodeChip_LotValidator_Validate(void);

#endif /* CODECHIP_LOTVALIDATOR_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
/* CodeChip temporary lot parameter provider (Trial 1st) */

#include "CodeChip_TempLotData.h"

static const AnalysisLotParam_t s_temp_lot_param =
{
  .lot_id = "TEMPLOT01",
  .raw_count = ANALYSIS_RAW_COUNT_MAX,
  .band_count = ANALYSIS_BAND_COUNT_MAX,
  .bands =
  {
    {
      .enabled = 1U,
      .is_control = 1U,
      .center_index = 340U,
      .search_range = 60U,
      .baseline_width = 40U,
      .peak_check = 4U,
      .cutoff_score = 1500,
      .control_cutoff_score = 1200,
      .coef_x1000 = 1000,
      .offset_score = 0,
      .name = "C"
    },
    {
      .enabled = 1U,
      .is_control = 0U,
      .center_index = 760U,
      .search_range = 60U,
      .baseline_width = 40U,
      .peak_check = 4U,
      .cutoff_score = 900,
      .control_cutoff_score = 1200,
      .coef_x1000 = 1000,
      .offset_score = 0,
      .name = "A"
    },
    {
      .enabled = 1U,
      .is_control = 0U,
      .center_index = 1180U,
      .search_range = 60U,
      .baseline_width = 40U,
      .peak_check = 4U,
      .cutoff_score = 900,
      .control_cutoff_score = 1200,
      .coef_x1000 = 1000,
      .offset_score = 0,
      .name = "B"
    },
    {
      .enabled = 0U,
      .is_control = 0U,
      .center_index = 1600U,
      .search_range = 60U,
      .baseline_width = 40U,
      .peak_check = 4U,
      .cutoff_score = 900,
      .control_cutoff_score = 1200,
      .coef_x1000 = 1000,
      .offset_score = 0,
      .name = "R"
    }
  }
};

int32_t CodeChip_TempLotData_GetLotParam(AnalysisLotParam_t* out_lot)
{
  if (out_lot == (AnalysisLotParam_t*)0)
  {
    return -1;
  }

  *out_lot = s_temp_lot_param;
  return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

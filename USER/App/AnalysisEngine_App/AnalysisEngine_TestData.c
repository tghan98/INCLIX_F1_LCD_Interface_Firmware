/* Analysis engine test data (F1 Trial 1st) */

/* 합성 파형 설계 근거:
 *   - 베이스라인: 100
 *   - band 0 (COV, control): index 120 근방, 최대 raw ~350, POS 예상
 *   - band 1 (A):            index 320 근방, 최대 raw ~280, POS 예상
 *   - band 2 (B):            index 520 근방, 최대 raw ~110, NEG 예상 (cutoff 이하)
 *   - band 3 (C):            enabled=0, 사용 안 함
 * 파형은 peak 영역에만 삼각형 근사로 올리고, 나머지 구간은 baseline 100을 유지한다.
 * 원본 고정 테이블은 designated initializer 미지정 구간이 0으로 남는 문제가 있어서,
 * 1차 Trial의 실제 계산 경로에서 바로 쓸 수 있도록 런타임 초기화로 전환한다.
 */

#ifdef USE_TEST_RAW_DATA

#include "App/AnalysisEngine_App/AnalysisEngine_TestData.h"

static RawSample_t s_test_raw[ANALYSIS_RAW_COUNT_MAX];
static uint8_t     s_test_raw_ready;

static const AnalysisLotParam_t s_test_lot =
{
  .lot_id    = "TEST_LOT_0001",
  .raw_count = ANALYSIS_RAW_COUNT_MAX,
  .band_count = 3U,
  .bands =
  {
    {
      .enabled              = 1U,
      .is_control           = 1U,
      .center_index         = 120U,
      .search_range         = 30U,
      .baseline_width       = 30U,
      .peak_check           = 4U,
      .cutoff_score         = 5000L,
      .control_cutoff_score = 5000L,
      .coef_x1000           = 1000L,
      .offset_score         = 0L,
      .name                 = "COV"
    },
    {
      .enabled              = 1U,
      .is_control           = 0U,
      .center_index         = 320U,
      .search_range         = 30U,
      .baseline_width       = 30U,
      .peak_check           = 4U,
      .cutoff_score         = 3000L,
      .control_cutoff_score = 0L,
      .coef_x1000           = 1000L,
      .offset_score         = 0L,
      .name                 = "A"
    },
    {
      .enabled              = 1U,
      .is_control           = 0U,
      .center_index         = 520U,
      .search_range         = 30U,
      .baseline_width       = 30U,
      .peak_check           = 4U,
      .cutoff_score         = 10000L,
      .control_cutoff_score = 0L,
      .coef_x1000           = 1000L,
      .offset_score         = 0L,
      .name                 = "B"
    },
    {
      .enabled = 0U
    }
  }
};

static void AnalysisEngine_TestData_InitRaw(void)
{
  uint16_t i;

  if (s_test_raw_ready != 0U)
  {
    return;
  }

  for (i = 0U; i < ANALYSIS_RAW_COUNT_MAX; i++)
  {
    s_test_raw[i] = (RawSample_t)100U;
  }

  for (i = 0U; i < 20U; i++)
  {
    s_test_raw[i] = (RawSample_t)0U;
  }

  /* 삼각형 peak 생성 헬퍼: center ± 20 구간에만 높이를 얹는다. */
  {
    const uint16_t centers[3] = { 120U, 320U, 520U };
    const uint16_t amps[3]    = { 250U, 180U, 10U };
    uint8_t band;

    for (band = 0U; band < 3U; band++)
    {
      int16_t offset;

      for (offset = -20; offset <= 20; offset++)
      {
        int32_t idx = (int32_t)centers[band] + (int32_t)offset;
        uint16_t abs_offset = (offset < 0) ? (uint16_t)(-offset) : (uint16_t)offset;
        uint16_t height;

        if ((idx < 0) || (idx >= (int32_t)ANALYSIS_RAW_COUNT_MAX))
        {
          continue;
        }

        height = (uint16_t)(((uint32_t)amps[band] * (uint32_t)(20U - abs_offset)) / 20U);
        s_test_raw[(uint16_t)idx] = (RawSample_t)(100U + height);
      }
    }
  }

  s_test_raw_ready = 1U;
}

int32_t AnalysisEngine_TestData_GetRawData(
    const RawSample_t** out_raw,
    uint16_t*           out_count)
{
  if ((out_raw == (const RawSample_t**)0) || (out_count == (uint16_t*)0))
  {
    return -1;
  }

  AnalysisEngine_TestData_InitRaw();

  *out_raw   = s_test_raw;
  *out_count = (uint16_t)ANALYSIS_RAW_COUNT_MAX;

  return 0;
}

int32_t AnalysisEngine_TestData_GetLotParam(
    const AnalysisLotParam_t** out_lot)
{
  if (out_lot == (const AnalysisLotParam_t**)0)
  {
    return -1;
  }

  *out_lot = &s_test_lot;

  return 0;
}

#endif /* USE_TEST_RAW_DATA */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

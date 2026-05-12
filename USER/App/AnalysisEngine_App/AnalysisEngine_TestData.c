/* Analysis engine test data (F1 Trial 1st) */

/* 합성 파형 설계 근거:
 *   - 베이스라인: ~100
 *   - band 0 (COV, control): index 120 근방, 최대 raw ~350, POS 예상
 *   - band 1 (A):            index 320 근방, 최대 raw ~280, POS 예상
 *   - band 2 (B):            index 520 근방, 최대 raw ~110, NEG 예상 (cutoff 이하)
 *   - band 3 (C):            enabled=0, 사용 안 함
 * 파형은 peak 영역에만 가우스 형태로 올림. 나머지 구간은 baseline 100 유지.
 * LotParam의 cutoff_score는 height-sum 기준으로 설정한다.
 *   COV cutoff 5000 → 최대 score 약 14000 → POS
 *   A   cutoff 3000 → 최대 score 약  9000 → POS
 *   B   cutoff 10000 → 최대 score 약 400  → NEG (일부러 임계값을 높임)
 * search_range, baseline_width는 F100 기준 30을 채용.
 * center_index 는 optical-black 18 + 명령 2 = 20 offset 후 수치로 설정함.
 * 유의: 이 값들은 알고리즘 경로 검증용 합성값이며 실제 광학 스펙이 아니다.
 */

#ifdef USE_TEST_RAW_DATA

#include "App/AnalysisEngine_App/AnalysisEngine_TestData.h"

/* ── 합성 raw 파형 ─────────────────────────────────────────────────────────
 * 2090 포인트 uint16_t 배열.
 * 앞 20 포인트(optical-black + command): 0
 * 나머지: baseline 100
 * peak 구간: center ± 20 범위에서 아래 공식으로 올림
 *   height(i) = peak_amp * (20 - |i - center|) / 20   (≈ 삼각 근사)
 */

/* Python으로 계산한 값을 정적 테이블로 표현.
 * 형식: [index] = value (중간 생략 구간은 100으로 채움)
 * 편의상 배열 이니셜라이저는 baseline 100 전체를 먼저 설정 후
 * designated initializer로 peak 구간만 덮어쓴다.
 * C99 designated initializer 사용 (IAR C99 이상 지원). */

/* band 0: COV, center=120, peak_amp=250, height(i)=100 + 250*(20-|i-120|)/20  */
/* band 1: A,   center=320, peak_amp=180, height(i)=100 + 180*(20-|i-320|)/20  */
/* band 2: B,   center=520, peak_amp= 10, height(i)=100 +  10*(20-|i-520|)/20  */

static const RawSample_t s_test_raw[2090U] =
{
  /* 0–19: optical-black + command 영역, 0으로 채움 */
  [  0] =  0, [  1] =  0, [  2] =  0, [  3] =  0, [  4] =  0,
  [  5] =  0, [  6] =  0, [  7] =  0, [  8] =  0, [  9] =  0,
  [ 10] =  0, [ 11] =  0, [ 12] =  0, [ 13] =  0, [ 14] =  0,
  [ 15] =  0, [ 16] =  0, [ 17] =  0, [ 18] =  0, [ 19] =  0,

  /* 20–2089: 나머지 기본값 100 (designated initializer 미지정 항목) */

  /* band 0 COV peak (center=120, amp=250) ─────────────────────────────── */
  [100] = 100, [101] = 113, [102] = 125, [103] = 138, [104] = 150,
  [105] = 163, [106] = 175, [107] = 188, [108] = 200, [109] = 213,
  [110] = 225, [111] = 238, [112] = 250, [113] = 263, [114] = 275,
  [115] = 288, [116] = 300, [117] = 313, [118] = 325, [119] = 338,
  [120] = 350, /* ← COV center, raw peak */
  [121] = 338, [122] = 325, [123] = 313, [124] = 300, [125] = 288,
  [126] = 275, [127] = 263, [128] = 250, [129] = 238, [130] = 225,
  [131] = 213, [132] = 200, [133] = 188, [134] = 175, [135] = 163,
  [136] = 150, [137] = 138, [138] = 125, [139] = 113,

  /* band 1 A peak (center=320, amp=180) ───────────────────────────────── */
  [300] = 100, [301] = 109, [302] = 118, [303] = 127, [304] = 136,
  [305] = 145, [306] = 154, [307] = 163, [308] = 172, [309] = 181,
  [310] = 190, [311] = 199, [312] = 208, [313] = 217, [314] = 226,
  [315] = 235, [316] = 244, [317] = 253, [318] = 262, [319] = 271,
  [320] = 280, /* ← A center, raw peak */
  [321] = 271, [322] = 262, [323] = 253, [324] = 244, [325] = 235,
  [326] = 226, [327] = 217, [328] = 208, [329] = 199, [330] = 190,
  [331] = 181, [332] = 172, [333] = 163, [334] = 154, [335] = 145,
  [336] = 136, [337] = 127, [338] = 118, [339] = 109,

  /* band 2 B peak (center=520, amp=10, 의도적으로 낮음 → NEG) ─────────── */
  [500] = 100, [501] = 101, [502] = 101, [503] = 101, [504] = 101,
  [505] = 102, [506] = 102, [507] = 102, [508] = 103, [509] = 103,
  [510] = 103, [511] = 104, [512] = 104, [513] = 104, [514] = 105,
  [515] = 105, [516] = 105, [517] = 106, [518] = 106, [519] = 106,
  [520] = 110, /* ← B center, raw peak */
  [521] = 106, [522] = 106, [523] = 106, [524] = 105, [525] = 105,
  [526] = 105, [527] = 104, [528] = 104, [529] = 104, [530] = 103,
  [531] = 103, [532] = 103, [533] = 102, [534] = 102, [535] = 102,
  [536] = 101, [537] = 101, [538] = 101, [539] = 100
  /* 미지정 나머지: C99 규칙에 따라 0이 아님 — IAR은 전역/정적 배열에서
   * 미지정 항목을 0으로 초기화하므로 baseline 100을 별도 보정한다. */
  /* NOTICE: C 표준에서 partially-initialized 정적 배열의 나머지는 0.
   *         따라서 위 표에서 명시되지 않은 20–2089 구간은 0이 된다.
   *         실제 파형 검증 시에는 AnalysisEngine_TestData_GetRawData 가
   *         반환하는 배열 전체를 baseline=100 으로 패치한 별도 RAM 버퍼를
   *         사용하도록 수정하거나, 배열을 명시적으로 100으로 채워야 한다.
   *         TODO (Commit 2 전 수정): RAM 버퍼 패치 방식으로 전환.      */
};

/* ── 임시 LotParam ────────────────────────────────────────────────────────── */

static const AnalysisLotParam_t s_test_lot =
{
  .lot_id    = "TEST_LOT_0001",
  .raw_count = 2090U,
  .band_count = 3U,
  .bands =
  {
    /* band 0: COV (control band) */
    {
      .enabled              = 1U,
      .is_control           = 1U,
      .center_index         = 120U,
      .search_range         = 30U,
      .baseline_width       = 30U,
      .peak_check           = 4U,
      .cutoff_score         = 5000L,
      .control_cutoff_score = 5000L,
      .coef_x1000           = 1000L,  /* 예약 (×1.000) */
      .offset_score         = 0L,     /* 예약 */
      .name                 = "COV"
    },
    /* band 1: A */
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
    /* band 2: B */
    {
      .enabled              = 1U,
      .is_control           = 0U,
      .center_index         = 520U,
      .search_range         = 30U,
      .baseline_width       = 30U,
      .peak_check           = 4U,
      .cutoff_score         = 10000L,  /* 의도적으로 높게 설정 → NEG */
      .control_cutoff_score = 0L,
      .coef_x1000           = 1000L,
      .offset_score         = 0L,
      .name                 = "B"
    },
    /* band 3: 미사용 */
    {
      .enabled = 0U
    }
  }
};

/* ── 함수 구현 ────────────────────────────────────────────────────────────── */

int32_t AnalysisEngine_TestData_GetRawData(
    const RawSample_t** out_raw,
    uint16_t*           out_count)
{
  if ((out_raw == (const RawSample_t**)0) || (out_count == (uint16_t*)0))
  {
    return -1;
  }

  *out_raw   = s_test_raw;
  *out_count = (uint16_t)(sizeof(s_test_raw) / sizeof(s_test_raw[0]));

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

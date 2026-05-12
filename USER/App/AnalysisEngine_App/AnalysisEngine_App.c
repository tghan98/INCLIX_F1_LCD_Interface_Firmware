/* Analysis engine application — qualitative strip analysis core (F1 Trial 1st) */

/*
 * F100 ImageAnalysis::analysisStrip(eQuality) 포팅 요약:
 *
 * [단계 1] diff 배열 계산: diff[i] = raw[i+3] - raw[i]
 * [단계 2] band별 peak 탐색: getPeak(center±range±peakCheck 범위에서 diff 부호 전환점)
 * [단계 3] baseline 직선 계산: baseStart/baseEnd = peakIdx ± baseline_width, y = ax+b (정수)
 * [단계 4] score 누적: Σ max(0, raw[x] - baseline_y) for x in [baseStart, baseEnd]
 * [단계 5] control 유효성 판단: control band score > control_cutoff_score
 * [단계 6] 정성 판정: score >= cutoff_score → POS (1차 Trial 보정 없음)
 * [단계 7] result_text 및 decision 채움
 *
 * TODO (후속 단계): 3-pass center shift(no=0/+5/+10) 후 max-score 채택 적용
 * TODO (후속 단계): coef_x1000 / offset_score 적용 (현재 예약 필드, 미사용)
 * TODO (후속 단계): CALCULATING 실행 시간 100ms 초과 시 baseline 나눗셈 회피 최적화 검토
 */

#include "App/AnalysisEngine_App/AnalysisEngine_App.h"
#include <string.h>

/* ── 내부 상수 ────────────────────────────────────────────────────────────── */

/* getPeak 외부 루프 상한 (F100 원본 21과 동일) */
#define PEAK_CHECK_MAX  21U

/* ── 모듈 내부 전용 정적 버퍼 ────────────────────────────────────────────── */
/*
 * diff 배열: raw_count - 3 개 유효. raw_count 최대 ANALYSIS_RAW_COUNT_MAX.
 * 함수 스코프 정적으로 선언하여 M0+ 스택 오버플로 방지.
 * AnalysisEngine_Run은 CALCULATING에서 1회만 호출되므로 재진입 문제 없음.
 */
static AnalysisCalc_t s_diff[ANALYSIS_RAW_COUNT_MAX];

/* ── 내부 헬퍼 함수 ───────────────────────────────────────────────────────── */

/*
 * getPeak — diff 배열에서 peak 전환점 인덱스를 반환한다.
 *
 * F100 원본 로직 직역:
 *   - check = peakCheck..20 루프를 돌며, 조건 충족 시마다 peakIndex 갱신.
 *   - 좌측: diff[i] > 0 이 check 개 연속 → 상승 에지 확인.
 *   - 전환점(peak): 처음으로 diff[i] <= 0 이 되는 i.
 *   - 우측: 전환점부터 diff[i] <= 0 이 check 개 연속 → 하강 에지 확인.
 *   - 두 조건 모두 만족 시 peakIndex = 전환점.
 *
 * @param start  탐색 시작 인덱스 (포함)
 * @param end    탐색 종료 인덱스 (미포함)
 * @param diff   차분 배열 포인터
 * @param diff_len diff 배열 유효 길이
 * @param peak_check  최소 연속 확인 수 (기본 4)
 * @return  peak 전환점 인덱스 (0이면 peak 없음)
 */
static uint16_t getPeak(
    uint16_t              start,
    uint16_t              end,
    const AnalysisCalc_t* diff,
    uint16_t              diff_len,
    uint8_t               peak_check)
{
  uint16_t peak_index = 0U;
  uint16_t check;

  /* 범위 클램프 */
  if (end > diff_len)
  {
    end = diff_len;
  }
  if (start >= end)
  {
    return 0U;
  }

  /* TODO (후속): check 루프를 peak_check 단일 실행으로 단순화 여부 검토 */
  for (check = (uint16_t)peak_check; check < PEAK_CHECK_MAX; check++)
  {
    uint16_t peak     = 0U;
    uint16_t left_cnt = 0U;
    uint16_t idx_l;

    /* 1) 상승 에지: diff > 0 연속 check 개 탐색 */
    for (idx_l = start; idx_l < end; idx_l++)
    {
      if (diff[idx_l] > (AnalysisCalc_t)0)
      {
        left_cnt++;
      }
      else
      {
        peak = idx_l;
        if (left_cnt >= check)
        {
          break;
        }
        else
        {
          left_cnt = 0U;
        }
      }
    }

    /* 2) 하강 에지: diff <= 0 연속 check 개 확인 */
    if (left_cnt >= check)
    {
      uint16_t right_cnt = 0U;
      uint16_t idx_r;

      for (idx_r = peak; idx_r < end; idx_r++)
      {
        if (diff[idx_r] <= (AnalysisCalc_t)0)
        {
          right_cnt++;
        }
        else
        {
          break;
        }
      }

      if (right_cnt >= check)
      {
        peak_index = peak;
      }
    }
  }

  return peak_index;
}

/*
 * fill_result_text — result_text 버퍼에 "{name}:{+/-}" 포맷 문자열을 채운다.
 *
 * @param buf     출력 버퍼 (길이 ANALYSIS_RESULT_TEXT_LEN 이상)
 * @param name    band 이름 (null 종결)
 * @param is_pos  1이면 '+', 0이면 '-'
 */
static void fill_result_text(char* buf, const char* name, uint8_t is_pos)
{
  uint8_t i = 0U;

  while ((name[i] != '\0') && (i < (ANALYSIS_BAND_NAME_LEN - 1U)))
  {
    buf[i] = name[i];
    i++;
  }
  buf[i] = ':';
  i++;
  buf[i] = is_pos ? '+' : '-';
  i++;
  buf[i] = '\0';
}

/* ── 공개 함수 구현 ───────────────────────────────────────────────────────── */

int32_t AnalysisEngine_Run(
    const AnalysisLotParam_t* lot,
    const RawSample_t*        raw,
    uint16_t                  raw_count,
    AnalysisResult_t*         out_result)
{
  uint16_t i;
  uint8_t  band_idx;
  uint8_t  ctrl_valid = 0U;

  /* ── 입력 가드 ───────────────────────────────────────────────────────── */
  if ((lot == (const AnalysisLotParam_t*)0) ||
      (raw == (const RawSample_t*)0)        ||
      (out_result == (AnalysisResult_t*)0)  ||
      (raw_count < 4U)                      ||
      (raw_count > ANALYSIS_RAW_COUNT_MAX))
  {
    return -1;
  }

  /* ── 결과 구조체 초기화 ──────────────────────────────────────────────── */
  (void)memset(out_result, 0, sizeof(AnalysisResult_t));

  /* ── [단계 1] diff 배열 계산: diff[i] = raw[i+3] - raw[i] ───────────── */
  /*
   * 유효 범위: i = 0 .. raw_count-4 (총 raw_count-3 개)
   * 나머지 끝 3개는 0으로 남겨 오버런을 막는다.
   */
  {
    uint16_t diff_count = raw_count - 3U;

    for (i = 0U; i < diff_count; i++)
    {
      s_diff[i] = (AnalysisCalc_t)raw[i + 3U] - (AnalysisCalc_t)raw[i];
    }
    /* 잔여 3개 0 초기화 */
    for (i = diff_count; i < diff_count + 3U; i++)
    {
      s_diff[i] = (AnalysisCalc_t)0;
    }
  }

  uint16_t diff_len = raw_count - 3U;

  /* ── [단계 2~7] band별 분석 ──────────────────────────────────────────── */
  for (band_idx = 0U; band_idx < ANALYSIS_BAND_COUNT_MAX; band_idx++)
  {
    const AnalysisBandParam_t* bp = &lot->bands[band_idx];

    /* 비활성화 band 건너뜀 */
    if (bp->enabled == 0U)
    {
      out_result->decision[band_idx]   = ANALYSIS_DECISION_INVALID;
      out_result->score[band_idx]      = 0U;
      out_result->peak_index[band_idx] = 0U;
      out_result->result_text[band_idx][0] = '\0';
      continue;
    }

    /* ── [단계 2] peak 탐색 ───────────────────────────────────────────── */
    /*
     * start = center - range - peakCheck
     * end   = center + range + peakCheck
     * start 가 0 이하이면 peak 탐색 생략 (F100 원본 "if(start > 0)" 동일)
     */
    uint16_t peak_idx = 0U;

    if (bp->center_index > (uint16_t)(bp->search_range + bp->peak_check))
    {
      uint16_t srch_start = bp->center_index
                            - (uint16_t)bp->search_range
                            - (uint16_t)bp->peak_check;
      uint16_t srch_end   = bp->center_index
                            + (uint16_t)bp->search_range
                            + (uint16_t)bp->peak_check;

      peak_idx = getPeak(srch_start, srch_end, s_diff, diff_len, bp->peak_check);
    }

    out_result->peak_index[band_idx] = peak_idx;

    if (peak_idx == 0U)
    {
      /* peak 없음: score=0, NEG */
      out_result->score[band_idx]    = 0U;
      out_result->decision[band_idx] = ANALYSIS_DECISION_NEGATIVE;
      fill_result_text(out_result->result_text[band_idx], bp->name, 0U);
      continue;
    }

    /* ── [단계 3] baseline 계산용 base_start / base_end 결정 ─────────── */
    /*
     * F100: baseStart = peakIdx - bandVal, baseEnd = peakIdx + bandVal
     * 인덱스가 raw 배열 범위를 벗어나지 않도록 클램프한다.
     */
    uint16_t base_start;
    uint16_t base_end;

    if (peak_idx > (uint16_t)bp->baseline_width)
    {
      base_start = peak_idx - (uint16_t)bp->baseline_width;
    }
    else
    {
      base_start = 0U;
    }

    base_end = peak_idx + (uint16_t)bp->baseline_width;
    if (base_end >= raw_count)
    {
      base_end = raw_count - 1U;
    }

    /* ── [단계 4] score 누적 ─────────────────────────────────────────── */
    /*
     * baseline 직선: y = raw[base_start] + slope_num*(x - base_start) / slope_den
     * slope_num = raw[base_end] - raw[base_start]  (AnalysisCalc_t)
     * slope_den = base_end - base_start            (uint16_t)
     *
     * height = raw[x] - baseline_y
     * score += height 이 양수인 경우만 누적
     *
     * 분모 0 가드: base_start == base_end 이면 slope=0 처리.
     */
    {
      AnalysisCalc_t slope_num = (AnalysisCalc_t)raw[base_end]
                               - (AnalysisCalc_t)raw[base_start];
      uint16_t       slope_den = base_end - base_start;
      AnalysisScore_t score_acc = 0U;
      uint16_t x;

      for (x = base_start; x <= base_end; x++)
      {
        AnalysisCalc_t baseline_y;
        AnalysisCalc_t height;

        if (slope_den == 0U)
        {
          /* 분모 0: 수평 baseline = raw[base_start] */
          baseline_y = (AnalysisCalc_t)raw[base_start];
        }
        else
        {
          /*
           * 정수 나눗셈: 절삭 오차 허용 (Trial 1)
           * TODO (후속): 실행 시간 > 100ms 시 나눗셈 회피 최적화 검토
           * 중간 곱셈 int64 승격: slope_num * (x - base_start) 오버플로 방지
           */
          baseline_y = (AnalysisCalc_t)raw[base_start]
                     + (AnalysisCalc_t)(
                         ((int64_t)slope_num * (int64_t)(x - base_start))
                         / (int64_t)slope_den
                       );
        }

        height = (AnalysisCalc_t)raw[x] - baseline_y;
        if (height > (AnalysisCalc_t)0)
        {
          score_acc += (AnalysisScore_t)height;
        }
      }

      out_result->score[band_idx] = score_acc;
    }

    /* ── [단계 5] control band 유효성 판단 ──────────────────────────── */
    if (bp->is_control != 0U)
    {
      if (out_result->score[band_idx] > (AnalysisScore_t)bp->control_cutoff_score)
      {
        ctrl_valid = 1U;
      }
    }

    /* ── [단계 6] 정성 판정 ──────────────────────────────────────────── */
    /*
     * 1차 Trial 판정식: score >= cutoff_score → POS, 아니면 NEG
     * TODO (후속): score * coef_x1000 / 1000 + offset_score >= cutoff_score 검토
     */
    {
      uint8_t is_pos = (out_result->score[band_idx] >= (AnalysisScore_t)bp->cutoff_score)
                       ? 1U : 0U;

      out_result->decision[band_idx] = is_pos
                                       ? ANALYSIS_DECISION_POSITIVE
                                       : ANALYSIS_DECISION_NEGATIVE;

      /* ── [단계 7] result_text 채움 ───────────────────────────────── */
      fill_result_text(out_result->result_text[band_idx], bp->name, is_pos);
    }
  }

  /* ── control band 유효성 최종 반영 ──────────────────────────────────── */
  /*
   * kit_valid = 0 이면 control band 미통과 → 비-control band 판정을 INVALID로 교체.
   * result_text는 "?" 표시로 덮어쓴다.
   */
  out_result->kit_valid = ctrl_valid;

  if (ctrl_valid == 0U)
  {
    for (band_idx = 0U; band_idx < ANALYSIS_BAND_COUNT_MAX; band_idx++)
    {
      if (lot->bands[band_idx].enabled == 0U)
      {
        continue;
      }
      if (lot->bands[band_idx].is_control == 0U)
      {
        out_result->decision[band_idx] = ANALYSIS_DECISION_INVALID;

        /* result_text: "{name}:?" */
        {
          const char* name = lot->bands[band_idx].name;
          uint8_t     j    = 0U;
          char*       buf  = out_result->result_text[band_idx];

          while ((name[j] != '\0') && (j < (ANALYSIS_BAND_NAME_LEN - 1U)))
          {
            buf[j] = name[j];
            j++;
          }
          buf[j++] = ':';
          buf[j++] = '?';
          buf[j]   = '\0';
        }
      }
    }
  }

  return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

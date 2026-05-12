/* Analysis engine shared data types (F1 Trial 1st) */

#ifndef ANALYSISENGINE_TYPES_H
#define ANALYSISENGINE_TYPES_H

#include <stdint.h>

/* ── 스캔 데이터 크기 상수 ───────────────────────────────────────────────── */

/* F100 광학 보드 기준 raw scan point 수 (SerialProtocol.h 참조) */
#define ANALYSIS_RAW_COUNT_MAX    2090U

/* 분석 대상 band 최대 수 (F100 TOTAL_BAND 기준) */
#define ANALYSIS_BAND_COUNT_MAX   4U

/* band 이름 문자열 최대 길이 (null 포함) */
#define ANALYSIS_BAND_NAME_LEN    8U

/* result_text 최대 길이 (null 포함), 포맷 "{band_name}:{+/-}" 수용 */
#define ANALYSIS_RESULT_TEXT_LEN  12U

/* ── 의미 기반 타입 alias ─────────────────────────────────────────────────── */

/* raw sample: 광학 보드 ADC 미가공값, 음수 없음 */
typedef uint16_t RawSample_t;

/* 중간 계산: diff, baseline numerator, height 등 음수 가능 */
typedef int32_t  AnalysisCalc_t;

/* score 누적: 음수 없음 */
typedef uint32_t AnalysisScore_t;

/* ── Band 파라미터 ────────────────────────────────────────────────────────── */

typedef struct
{
  uint8_t  enabled;                           /* 이 band 사용 여부 */
  uint8_t  is_control;                        /* 1이면 control band */
  uint16_t center_index;                      /* raw 배열 내 band 중심 인덱스 */
  uint16_t search_range;                      /* center ± search_range 탐색 */
  uint16_t baseline_width;                    /* peak 기준 ±baseline_width 로 base 결정 */
  uint8_t  peak_check;                        /* getPeak 에서 연속 확인 count (기본 4) */
  int32_t  cutoff_score;                      /* 정성 판정 임계값 (raw score 단위) */
  int32_t  control_cutoff_score;              /* control band 유효 임계값 (raw score 단위) */

  /* TODO: 후속 fixed-point 보정 단계에서 도입 — 1차 Trial 미사용 예약 필드 */
  int32_t  coef_x1000;                        /* coef * 1000 정수 스케일링 (예약) */
  int32_t  offset_score;                      /* offset (raw score 단위, 예약) */

  char     name[ANALYSIS_BAND_NAME_LEN];      /* band 표시 이름 (예: "COV", "A", "B") */
} AnalysisBandParam_t;

/* ── Lot 파라미터 ─────────────────────────────────────────────────────────── */

typedef struct
{
  char                  lot_id[16];                              /* lot 식별자 */
  uint16_t              raw_count;                               /* 유효 raw 포인트 수 */
  uint8_t               band_count;                              /* 실제 사용 band 수 */
  AnalysisBandParam_t   bands[ANALYSIS_BAND_COUNT_MAX];
} AnalysisLotParam_t;

/* ── 판정 enum ────────────────────────────────────────────────────────────── */

typedef enum
{
  ANALYSIS_DECISION_INVALID  = 0,
  ANALYSIS_DECISION_NEGATIVE,
  ANALYSIS_DECISION_POSITIVE
} AnalysisDecision_t;

/* ── 분석 결과 ────────────────────────────────────────────────────────────── */

typedef struct
{
  uint8_t            kit_valid;                                  /* control band 유효 여부 */
  AnalysisScore_t    score[ANALYSIS_BAND_COUNT_MAX];             /* band 별 누적 score */
  uint16_t           peak_index[ANALYSIS_BAND_COUNT_MAX];        /* band 별 peak 위치 */
  AnalysisDecision_t decision[ANALYSIS_BAND_COUNT_MAX];          /* band 별 정성 판정 (필수) */

  /* LCD 표시 편의 보조 필드 — 포맷: "{band_name}:{+/-}" (예: "COV:+", "A:-")
   * 후속 단계에서 decision enum 기반 표시 책임 분리를 검토한다.
   * BLE/이력 저장 등 후속 소비자는 decision만 사용하고 이 필드는 무시한다. */
  char               result_text[ANALYSIS_BAND_COUNT_MAX][ANALYSIS_RESULT_TEXT_LEN];
} AnalysisResult_t;

#endif /* ANALYSISENGINE_TYPES_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

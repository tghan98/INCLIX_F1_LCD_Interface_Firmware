/* Analysis engine application — qualitative strip analysis core (F1 Trial 1st) */

#ifndef ANALYSISENGINE_APP_H
#define ANALYSISENGINE_APP_H

#include "main.h"
#include "App/AnalysisEngine_App/AnalysisEngine_Types.h"

/**
 * @brief  정성 strip 분석 1회 실행.
 *         입력 출처(TestData / 실제 광학 보드 / 실제 CodeChip)를 알지 않는다.
 *         SequenceManager의 CALCULATING 상태에서 1회 호출한다.
 *
 * @param  lot        LotParam const 포인터 (SequenceManager 캐시에서 전달)
 * @param  raw        RawSample_t 배열 const 포인터
 * @param  raw_count  배열 내 유효 샘플 수 (최대 ANALYSIS_RAW_COUNT_MAX)
 * @param  out_result 결과 구조체 포인터 (SequenceManager의 s_last_result)
 *
 * @retval  0  정상 완료 (kit_valid 여부와 무관)
 * @retval -1  파라미터 NULL 또는 범위 이상
 */
int32_t AnalysisEngine_Run(
    const AnalysisLotParam_t* lot,
    const RawSample_t*        raw,
    uint16_t                  raw_count,
    AnalysisResult_t*         out_result
);

#endif /* ANALYSISENGINE_APP_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

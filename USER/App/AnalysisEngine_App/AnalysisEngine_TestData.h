/* Analysis engine test data provider (F1 Trial 1st) */

/* 1차 Trial 전용 테스트 RawData 공급 모듈.
 * USE_TEST_RAW_DATA 매크로로 조건부 컴파일한다.
 * 광학 보드 도입 후에는 OpticalSampler가 동일 역할을 대체하도록 한다.
 * TODO: OpticalSampler 도입 시 이 파일을 빌드 대상에서 제거하고
 *       AnalysisEngine_TestData_GetRawData 호출부를 OpticalSampler_GetRawData 로 교체한다. */

#ifndef ANALYSISENGINE_TESTDATA_H
#define ANALYSISENGINE_TESTDATA_H

#ifdef USE_TEST_RAW_DATA

#include <stdint.h>
#include "App/AnalysisEngine_App/AnalysisEngine_Types.h"

/**
 * @brief 1차 Trial용 테스트 RawData 포인터와 샘플 수를 반환한다.
 * @param out_raw   테스트 raw 배열의 const 포인터를 저장할 출력 포인터.
 * @param out_count 배열 내 유효 샘플 수를 저장할 출력 포인터.
 * @retval  0 성공.
 * @retval -1 출력 포인터가 NULL.
 */
int32_t AnalysisEngine_TestData_GetRawData(
    const RawSample_t** out_raw,
    uint16_t*           out_count
);

/**
 * @brief 1차 Trial용 임시 LotParam 포인터를 반환한다.
 *        실제 CodeChip 포맷 확정 후 CodeChip_Parser로 교체한다.
 * @param out_lot LotParam의 const 포인터를 저장할 출력 포인터.
 * @retval  0 성공.
 * @retval -1 출력 포인터가 NULL.
 */
int32_t AnalysisEngine_TestData_GetLotParam(
    const AnalysisLotParam_t** out_lot
);

#endif /* USE_TEST_RAW_DATA */

#endif /* ANALYSISENGINE_TESTDATA_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

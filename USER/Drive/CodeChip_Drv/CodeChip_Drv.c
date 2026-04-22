/* CodeChip driver — board-dependent Micro SD card detect GPIO polling */

#include "CodeChip_Drv.h"

/**
 * @brief CodeChip detect 드라이버를 초기화한다.
 * @details GPIO 핀은 CubeMX에서 Input으로 설정되어 있어야 한다.
 *          추가 초기화가 필요하면 여기에 작성한다.
 */
void CodeChip_Drv_Init(void)
{
  /* no-op: GPIO initialization is handled by CubeMX-generated code */
}

/**
 * @brief SD card detect 핀의 현재 GPIO 레벨을 반환한다.
 * @return GPIO_PIN_RESET 또는 GPIO_PIN_SET.
 */
GPIO_PinState CodeChip_Drv_ReadDetectPin(void)
{
  return HAL_GPIO_ReadPin(CODECHIP_DRV_DETECT_PORT, CODECHIP_DRV_DETECT_PIN);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

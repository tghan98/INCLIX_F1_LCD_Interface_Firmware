#ifndef __EEPROM_CAT24C256_DRV_H__
#define __EEPROM_CAT24C256_DRV_H__

#include "main.h"

#define EEPROM_CAT24C256_TOTAL_BYTES    32768U
#define EEPROM_CAT24C256_PAGE_SIZE      64U

HAL_StatusTypeDef EEPROM_CAT24C256_Drv_WaitReady(uint32_t timeout_ms);
HAL_StatusTypeDef EEPROM_CAT24C256_Drv_WritePage(uint16_t mem_addr, const uint8_t *data, uint16_t len);
HAL_StatusTypeDef EEPROM_CAT24C256_Drv_Read(uint16_t mem_addr, uint8_t *data, uint16_t len);
HAL_StatusTypeDef EEPROM_CAT24C256_Drv_ProbeAddress(uint8_t addr_7bit, uint32_t timeout_ms);
HAL_StatusTypeDef EEPROM_CAT24C256_Drv_WriteByte(uint16_t mem_addr, uint8_t data);
HAL_StatusTypeDef EEPROM_CAT24C256_Drv_WriteByteRaw(uint16_t mem_addr, uint8_t data);
uint32_t EEPROM_CAT24C256_Drv_GetLastError(void);

#endif /* __EEPROM_CAT24C256_DRV_H__ */

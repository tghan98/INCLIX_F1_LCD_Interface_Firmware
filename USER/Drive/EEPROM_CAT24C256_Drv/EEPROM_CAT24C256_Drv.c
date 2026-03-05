#include "EEPROM_CAT24C256_Drv.h"

#define EEPROM_CAT24C256_I2C_ADDR       (0x51U << 1)
#define EEPROM_CAT24C256_READY_TRIALS   100U
#define EEPROM_CAT24C256_TIMEOUT_MS     500U
#define EEPROM_CAT24C256_WRITE_DELAY_MS 10U

extern I2C_HandleTypeDef hi2c1;
static uint32_t s_last_i2c_error;

HAL_StatusTypeDef EEPROM_CAT24C256_Drv_WaitReady(uint32_t timeout_ms)
{
  HAL_StatusTypeDef hal_status;

  hal_status = HAL_I2C_IsDeviceReady(&hi2c1,
                                     EEPROM_CAT24C256_I2C_ADDR,
                                     EEPROM_CAT24C256_READY_TRIALS,
                                     timeout_ms);
  s_last_i2c_error = HAL_I2C_GetError(&hi2c1);
  return hal_status;
}

HAL_StatusTypeDef EEPROM_CAT24C256_Drv_ProbeAddress(uint8_t addr_7bit, uint32_t timeout_ms)
{
  HAL_StatusTypeDef hal_status;

  hal_status = HAL_I2C_IsDeviceReady(&hi2c1,
                                     (uint16_t)(addr_7bit << 1),
                                     EEPROM_CAT24C256_READY_TRIALS,
                                     timeout_ms);
  s_last_i2c_error = HAL_I2C_GetError(&hi2c1);
  return hal_status;
}

HAL_StatusTypeDef EEPROM_CAT24C256_Drv_WritePage(uint16_t mem_addr, const uint8_t *data, uint16_t len)
{
  HAL_StatusTypeDef hal_status;
  uint16_t page_offset;

  if ((data == 0) || (len == 0U) || (len > EEPROM_CAT24C256_PAGE_SIZE))
  {
    return HAL_ERROR;
  }

  page_offset = (uint16_t)(mem_addr % EEPROM_CAT24C256_PAGE_SIZE);
  if ((page_offset + len) > EEPROM_CAT24C256_PAGE_SIZE)
  {
    return HAL_ERROR;
  }

  hal_status = HAL_I2C_Mem_Write(&hi2c1,
                                 EEPROM_CAT24C256_I2C_ADDR,
                                 mem_addr,
                                 I2C_MEMADD_SIZE_16BIT,
                                 (uint8_t *)data,
                                 len,
                                 EEPROM_CAT24C256_TIMEOUT_MS);
  s_last_i2c_error = HAL_I2C_GetError(&hi2c1);
  if (hal_status != HAL_OK)
  {
    return hal_status;
  }

  HAL_Delay(EEPROM_CAT24C256_WRITE_DELAY_MS);
  s_last_i2c_error = HAL_I2C_GetError(&hi2c1);
  return HAL_OK;
}

HAL_StatusTypeDef EEPROM_CAT24C256_Drv_WriteByte(uint16_t mem_addr, uint8_t data)
{
  return EEPROM_CAT24C256_Drv_WritePage(mem_addr, &data, 1U);
}

HAL_StatusTypeDef EEPROM_CAT24C256_Drv_WriteByteRaw(uint16_t mem_addr, uint8_t data)
{
  HAL_StatusTypeDef hal_status;
  uint8_t tx_buf[3];

  tx_buf[0] = (uint8_t)((mem_addr >> 8) & 0xFFU);
  tx_buf[1] = (uint8_t)(mem_addr & 0xFFU);
  tx_buf[2] = data;

  hal_status = HAL_I2C_Master_Transmit(&hi2c1,
                                       EEPROM_CAT24C256_I2C_ADDR,
                                       tx_buf,
                                       (uint16_t)sizeof(tx_buf),
                                       EEPROM_CAT24C256_TIMEOUT_MS);
  s_last_i2c_error = HAL_I2C_GetError(&hi2c1);
  if (hal_status != HAL_OK)
  {
    return hal_status;
  }

  HAL_Delay(EEPROM_CAT24C256_WRITE_DELAY_MS);
  s_last_i2c_error = HAL_I2C_GetError(&hi2c1);
  return HAL_OK;
}

HAL_StatusTypeDef EEPROM_CAT24C256_Drv_Read(uint16_t mem_addr, uint8_t *data, uint16_t len)
{
  if ((data == 0) || (len == 0U))
  {
    return HAL_ERROR;
  }

  {
    HAL_StatusTypeDef hal_status;

    hal_status = HAL_I2C_Mem_Read(&hi2c1,
                                  EEPROM_CAT24C256_I2C_ADDR,
                                  mem_addr,
                                  I2C_MEMADD_SIZE_16BIT,
                                  data,
                                  len,
                                  EEPROM_CAT24C256_TIMEOUT_MS);
    s_last_i2c_error = HAL_I2C_GetError(&hi2c1);
    return hal_status;
  }
}

uint32_t EEPROM_CAT24C256_Drv_GetLastError(void)
{
  return s_last_i2c_error;
}

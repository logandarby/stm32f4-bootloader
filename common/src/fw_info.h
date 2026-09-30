#ifndef A70016D1_0ACE_480C_BAC3_CDD90AD395D5
#define A70016D1_0ACE_480C_BAC3_CDD90AD395D5

/**
 * The firmware_info_t struct is added to a firmware image after the vector
 * interrupt table with the sentinel and device_id. The rest are computed
 * on transfer time and stored in the device
 */

#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/memorymap.h>

#include "common.h"

#define FW_INFO_SENTINEL (0xC0FFEE00U)
#define FW_CRC_VALIDATE_LEN(fw_length) \
  ((fw_length) - sizeof(vector_table_t) - sizeof(firmware_info_t))
#define FW_CRC_MAX_LEN                                     \
  (FLASH_SIZE - BOOTLOADER_SIZE - sizeof(vector_table_t) - \
   sizeof(firmware_info_t))

PACKED_STRUCT_BEGIN
typedef struct {
  uint32_t sentinel;
  uint32_t device_id;
  uint32_t version;
  uint32_t length;
  uint32_t _reserved0;
  uint32_t _reserved1;
  uint32_t _reserved2;
  uint32_t _reserved3;
  uint32_t _reserved4;
  uint32_t crc32;
  uint8_t signature[2420];
} firmware_info_t;
PACKED_STRUCT_END

#endif /* A70016D1_0ACE_480C_BAC3_CDD90AD395D5 */

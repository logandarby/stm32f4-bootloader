#include "firmware_validation.h"

#include "crc.h"
#include "fw_info.h"

bool fw_validate_firmware_image(void) {
  const firmware_info_t* fw_info = (const firmware_info_t*)FW_INFO_ADDR;
  if (fw_info->sentinel != FW_INFO_SENTINEL ||
      fw_info->device_id != DEVICE_ID) {
    return false;
  }
  const uint32_t* start_addr = (const uint32_t*)FW_INFO_VALIDATE_FROM;
  crc32_reset();
  const size_t payload_wrds =
      FW_CRC_VALIDATE_LEN(fw_info->length) / sizeof(uint32_t);
  const uint32_t crc32 = crc32_update_block(start_addr, payload_wrds);
  crc32_reset();

  return crc32 == fw_info->crc32;
}
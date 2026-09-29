#include "firmware_validation.h"

#include "bl_metadata.h"
#include "crc.h"
#include "fw_info.h"
#include "memory.h"

// TODO: Add cryptovraphic signature post-quant

bool fw_validate_staged_firmware_image(void) {
  const firmware_info_t* fw_info = (const firmware_info_t*)LD_SLOT_B_START;
  if (fw_info->sentinel != FW_INFO_SENTINEL ||
      fw_info->device_id != DEVICE_ID ||
      fw_info->length > FIRMWARE_MAX_LEN) {
    return false;
  }

  crc32_reset();
  const size_t payload_wrds =
      FW_CRC_VALIDATE_LEN(MIN(fw_info->length, FIRMWARE_MAX_LEN)) /
      sizeof(uint32_t);
  const uint32_t crc32 =
      crc32_update_block((const uint32_t*)FW_STAGING_ADDR, payload_wrds);
  crc32_reset();
  return crc32 == fw_info->crc32;
}

bool fw_validate_active_firmware_image(void) {
  const firmware_info_t* fw_info =
      (const firmware_info_t*)FW_ACTIVE_INFO_ADDR;
  if (fw_info->sentinel != FW_INFO_SENTINEL ||
      fw_info->device_id != DEVICE_ID ||
      fw_info->length > FIRMWARE_MAX_LEN) {
    return false;
  }
  const uint32_t* start_addr =
      (const uint32_t*)FW_ACTIVE_INFO_VALIDATE_FROM;
  crc32_reset();
  const size_t payload_wrds =
      FW_CRC_VALIDATE_LEN(MIN(fw_info->length, FIRMWARE_MAX_LEN)) /
      sizeof(uint32_t);
  const uint32_t crc32 = crc32_update_block(start_addr, payload_wrds);
  crc32_reset();

  return crc32 == fw_info->crc32;
}
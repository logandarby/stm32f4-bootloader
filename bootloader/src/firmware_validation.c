#include "firmware_validation.h"

#include <mldsa_native.h>

#include "bl_metadata.h"
#include "crc.h"
#include "fw_info.h"
#include "key.h"
#include "memory.h"

// TODO: Add cryptovraphic signature post-quant

bool fw_validate_staged_firmware_image(void) {
  const firmware_info_t* fw_info =
      (const firmware_info_t*)FW_ACTIVE_INFO_ADDR;
  if (fw_info->sentinel != FW_INFO_SENTINEL ||
      fw_info->device_id != DEVICE_ID ||
      fw_info->length > FIRMWARE_MAX_LEN) {
    return false;
  }

  // verify crc
  crc32_reset();
  const size_t payload_len =
      FW_CRC_VALIDATE_LEN(MIN(fw_info->length, FIRMWARE_MAX_LEN));
  const uint32_t* start_addr =
      (const uint32_t*)FW_STAGING_INFO_VALIDATE_FROM;
  const uint32_t crc32 =
      crc32_update_block(start_addr, payload_len / sizeof(uint32_t));
  if (crc32 != fw_info->crc32) {
    return false;
  }

  // verify signature
  const int verify_result =
      mldsa_verify(fw_info->signature, (const uint8_t*)start_addr,
                   payload_len, NULL, 0, MLDSA44_PUBLIC_KEY);
  return verify_result == 0;
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
  const size_t payload_len =
      FW_CRC_VALIDATE_LEN(MIN(fw_info->length, FIRMWARE_MAX_LEN));
  const uint32_t crc32 =
      crc32_update_block(start_addr, payload_len / sizeof(uint32_t));

  if (crc32 != fw_info->crc32) {
    return false;
  }

  // Verify signature
  const int verify_result =
      mldsa_verify(fw_info->signature, (const uint8_t*)start_addr,
                   payload_len, NULL, 0, MLDSA44_PUBLIC_KEY);
  return verify_result == 0;
}
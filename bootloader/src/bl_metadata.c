#include "bl_metadata.h"

#include <string.h>

#include "bl_flash.h"
#include "crc.h"
#include "memory.h"

// Contains the default bootloader metadata.

SECTION(".bootloader_metadata")
bl_metadata_t bl_metadata = {
    .sentinel = BL_META_SENTINEL,
    .active_version = BL_FACTORY_FIRMWARE_VERSION,
    .staging_version = BL_FACTORY_FIRMWARE_VERSION,
    ._reserved =
        {
            BL_META_PADDING,
            BL_META_PADDING,
            BL_META_PADDING,
            BL_META_PADDING,
            BL_META_PADDING,
            BL_META_PADDING,
        },
    .active_crc32 = 0x0,
    .staging_crc32 = 0x0,
    .state = BLMetaState_CONFIRMED,
    .swap_state = BLSwapState_NONE,
    .swap_step = BLSwapStep_IDLE,
};

void bl_meta_rewrite(uint32_t active_ver, uint32_t staging_ver,
                     BLSwapState swap_state, BLMetaState app_state,
                     BLSwapStep swap_step) {
  bl_metadata_t new_meta = {
      .sentinel = BL_META_SENTINEL,
      .active_version = active_ver,
      .staging_version = staging_ver,
      .swap_state = swap_state,
      .state = app_state,
      .active_crc32 = 0x0,
      .staging_crc32 = 0x0,
      .swap_step = swap_step,
  };

  bl_flash_erase_metadata();  // Erases Sector 2 (0x0800 8000)
  bl_flash_write(LD_BOOT_METADATA_START, (const uint8_t*)&new_meta,
                 sizeof(bl_metadata_t));
}

static void bl_state_rewrite(BLSwapState swap_state, BLMetaState app_state,
                             BLSwapStep swap_step) {
  const bl_metadata_t* meta = bl_metadata_get();
  bl_metadata_t new_meta;
  memcpy(&new_meta, meta, sizeof(bl_metadata_t));
  new_meta.swap_state = swap_state;
  new_meta.state = app_state;
  new_meta.swap_state = swap_step;

  bl_flash_erase_metadata();
  bl_flash_write(LD_BOOT_METADATA_START, (const uint8_t*)&new_meta,
                 sizeof(bl_metadata_t));
}

// Main funcs

bool bl_meta_is_valid(void) {
  return bl_metadata.sentinel == BL_META_SENTINEL;
}

void bl_meta_set_swap_state(BLSwapState state) {
  if (state & ~bl_metadata.swap_state) {
    bl_state_rewrite(state, bl_metadata.state, bl_metadata.swap_step);
    return;
  }
  bl_flash_write((uint32_t)&bl_metadata.swap_state, (uint8_t*)&state,
                 sizeof(uint32_t));
}

void bl_meta_set_swap_step(BLSwapStep step) {
  if (step & ~bl_metadata.swap_step) {
    bl_state_rewrite(bl_metadata.swap_state, bl_metadata.state, step);
    return;
  }
  bl_flash_write((uint32_t)&bl_metadata.swap_step, (uint8_t*)&step,
                 sizeof(uint32_t));
}

void bl_meta_set_state(BLMetaState state) {
  if (state & ~bl_metadata.state) {
    bl_state_rewrite(bl_metadata.swap_state, state, bl_metadata.swap_step);
    return;
  }
  bl_flash_write((uint32_t)&bl_metadata.state, (uint8_t*)&state,
                 sizeof(uint32_t));
}

// void bl_meta_set_new_firmware(uint32_t active_version) {
//   bl_metadata_t new_meta = {0};
//   memcpy(&new_meta, &bl_metadata, sizeof(bl_metadata_t));
//   new_meta.staging_version = new_meta.active_version;
//   new_meta.active_version = active_version;
//   new_meta.state = BLMetaState_PENDING_TEST;
//   bl_flash_erase_metadata();
//   bl_flash_write(LD_BOOT_METADATA_START, (uint8_t*)&new_meta,
//                  sizeof(bl_metadata_t));
// }

const bl_metadata_t* bl_metadata_get(void) { return &bl_metadata; }
#include "bl_run.h"

#include <libopencm3/stm32/flash.h>

#include "bl_flash.h"
#include "bl_metadata.h"
#include "firmware_transfer.h"
#include "firmware_validation.h"
#include "inbuilt-button.h"
#include "led.h"
#include "memory.h"

typedef enum {
  BootState_START,
  BootState_HOST_TRANSFER,
  BootState_EVALUATE_META,
  BootState_EXECUTE_SWAP,
  BootState_EXECUTE_REVERT,
  BootState_COMPLETED
} BootState;

static void swap_engine_run(void) {
  // NOTE: All the if statements are necessary, to be able to continue if a
  // swap is in progress
  const bl_metadata_t* meta = bl_metadata_get();
  if (meta->swap_state == BLSwapState_PENDING) {
    bl_meta_set_swap_state(BLSwapState_IN_PROGRESS);
  }
  if (meta->swap_step == BLSwapStep_IDLE) {
    bl_flash_erase_firmware(BLFlashFWSlot_SCRATCH);
    bl_flash_write(LD_SLOT_SCRATCH_START, (const uint8_t*)LD_SLOT_A_START,
                   LD_MAX_SLOT_SIZE);
    bl_meta_set_swap_step(BLSwapStep_0_SCRATCH_DONE);
  }
  if (meta->swap_step == BLSwapStep_0_SCRATCH_DONE) {
    bl_flash_erase_firmware(BLFlashFWSlot_A);
    bl_flash_write(LD_SLOT_A_START, (const uint8_t*)LD_SLOT_B_START,
                   LD_MAX_SLOT_SIZE);
    bl_meta_set_swap_step(BLSwapStep_1_SLOTA_DONE);
  }
  if (meta->swap_step == BLSwapStep_1_SLOTA_DONE) {
    bl_flash_erase_firmware(BLFlashFWSlot_B);
    bl_flash_write(LD_SLOT_B_START, (const uint8_t*)LD_SLOT_SCRATCH_START,
                   LD_MAX_SLOT_SIZE);
    bl_meta_set_swap_step(BLSwapStep_2_SLOTB_DONE);
  }
  /* Post-Swap Cleanup & Flags */
  // TODO: Figure out versioning
  // flash_bitflip_write((uint32_t *)&meta->active_version,
  // meta->staging_version); flash_bitflip_write((uint32_t
  // *)&meta->active_crc32, meta->staging_crc32);
  bl_meta_set_swap_state(BLSwapState_COMPLETED);
}

static void revert_engine_run(void) {
  const bl_metadata_t* meta = bl_metadata_get();
  if (meta->swap_state != BLSwapState_REVERT_IN_PROG) {
    bl_meta_rewrite(meta->active_version, meta->staging_version,
                    BLSwapState_REVERT_IN_PROG, BLMetaState_CONFIRMED,
                    BLSwapStep_IDLE);
  }
  swap_engine_run();
  bl_meta_rewrite(meta->active_version, meta->staging_version,
                  BLSwapState_NONE, BLMetaState_CONFIRMED,
                  BLSwapStep_IDLE);

  // /* Restore active/staging version metadata parity */
  // bl_meta_set_staging_version(0);
  // bl_meta_set_staging_crc32(0);
}

static BootState evaluate_metadata_next_state(void) {
  if (!bl_meta_is_valid()) {
    return BootState_HOST_TRANSFER;
  }
  switch (bl_metadata_get()->swap_state) {
    case BLSwapState_PENDING:
    case BLSwapState_IN_PROGRESS:
      return BootState_EXECUTE_SWAP;
    case BLSwapState_REVERT_PENDING:
    case BLSwapState_REVERT_IN_PROG:
      return BootState_EXECUTE_REVERT;
    case BLSwapState_COMPLETED:
    case BLSwapState_NONE:
    default:
      break;  // Fall through
  }
  switch (bl_metadata_get()->state) {
    case BLMetaState_PENDING_TEST:
      bl_meta_set_swap_state(BLSwapState_REVERT_PENDING);
      return BootState_EXECUTE_REVERT;
    case BLMetaState_ROLLBACK_REQ:
      bl_meta_set_swap_state(BLSwapState_REVERT_PENDING);
      return BootState_EXECUTE_REVERT;
    case BLMetaState_CONFIRMED:
      if (fw_validate_active_firmware_image()) {
        return BootState_COMPLETED;
      } else {
        bl_meta_set_swap_state(BLSwapState_REVERT_PENDING);
        return BootState_EXECUTE_REVERT;
      }
    case BLMetaState_ERASED:
    default:
      return BootState_HOST_TRANSFER;
  }
}

void bootloader_run(void) {
  BootState state = BootState_START;

  while (1) {
    switch (state) {
      case BootState_START:
        state = inbuilt_button_is_pressed() ? BootState_HOST_TRANSFER
                                            : BootState_EVALUATE_META;
        break;

      case BootState_HOST_TRANSFER:
        led_set(LedState_ON);
        const bool fw_is_valid = firmware_transfer_start();
        if (fw_is_valid) {
          bl_meta_set_state(BLMetaState_PENDING_TEST);
          bl_meta_set_swap_state(BLSwapState_PENDING);
          state = BootState_EXECUTE_SWAP;
        }
        break;

      case BootState_EVALUATE_META:
        state = evaluate_metadata_next_state();
        break;

      case BootState_EXECUTE_SWAP:
        swap_engine_run(); /* SWP Engine executed here */
        bl_meta_set_state(BLMetaState_PENDING_TEST);
        state = BootState_COMPLETED;
        break;

      case BootState_EXECUTE_REVERT:
        revert_engine_run(); /* R-S Engine executed here */
        bl_meta_set_state(BLMetaState_CONFIRMED);
        state = BootState_COMPLETED;
        break;

      case BootState_COMPLETED:
        if (bl_metadata_get()->state == BLMetaState_PENDING_TEST) {
          // TODO:
          // iwdg_init_5s();
        }
        return;
    }
  }
}

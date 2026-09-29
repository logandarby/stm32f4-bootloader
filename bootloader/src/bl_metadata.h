#ifndef AB1B8087_A2E1_4E9F_9D58_10B4C15D95C4
#define AB1B8087_A2E1_4E9F_9D58_10B4C15D95C4

#include "common.h"

#define BL_META_SENTINEL (0x544F4F42U)
#define BL_META_PADDING (0xFFFFFFFFU)
#define BL_FACTORY_FIRMWARE_VERSION (1U)

#define BL_SWAP_BLOCK_SIZE (16U * 1024U)
#define BL_SWAP_BLOCKS_TOTAL (13U)

typedef enum : uint32_t {
  BLMetaState_ERASED = 0xFFFFFFFF,
  BLMetaState_PENDING_TEST = 0xFFFFFFFE,
  BLMetaState_CONFIRMED = 0xFFFFFFFC,
  BLMetaState_ROLLBACK_REQ = 0x00000000
} BLMetaState;

typedef enum : uint32_t {
  BLSwapState_NONE = 0xFFFFFFFF,
  BLSwapState_PENDING = 0xFFFFFFFE,
  BLSwapState_IN_PROGRESS = 0xFFFFFFFC,
  BLSwapState_REVERT_PENDING = 0xFFFFFFF0,
  BLSwapState_REVERT_IN_PROG = 0xFFFFFFE0,
  BLSwapState_COMPLETED = 0xFFFFFF00,
} BLSwapState;

typedef enum : uint32_t {
  BLSwapStep_IDLE = 0xFFFFFFFF,
  BLSwapStep_0_SCRATCH_DONE = 0xFFFFFFFE,
  BLSwapStep_1_SLOTA_DONE = 0xFFFFFFFC,
  BLSwapStep_2_SLOTB_DONE = 0xFFFFFFF8
} BLSwapStep;

PACKED_STRUCT_BEGIN
typedef struct {
  uint32_t sentinel;
  uint32_t active_version;
  uint32_t staging_version;
  uint32_t swap_state;
  uint32_t state;
  uint32_t active_crc32;
  uint32_t staging_crc32;
  uint32_t swap_step;
  uint32_t _reserved[6];
} bl_metadata_t;
PACKED_STRUCT_END

/**
 * Returns if the bootloader meta data is valid (proper CRC, etc)
 * REQUIRES: CRC peripheral is enabled
 */
bool bl_meta_is_valid(void);

/**
 * Change the state of the bl_metadata
 */
void bl_meta_set_state(BLMetaState state);

void bl_meta_set_swap_state(BLSwapState state);

void bl_meta_set_swap_step(BLSwapStep step);

/**
 * Change the versions of firmware stored in the metadata,
 * and sets the state back to STATE_PENDING_TEST
 */
void bl_meta_set_new_firmware(uint32_t new_version);

/**
 * Get the data inside
 */
const bl_metadata_t* bl_metadata_get(void);

#endif /* AB1B8087_A2E1_4E9F_9D58_10B4C15D95C4 */

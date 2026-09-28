#ifndef AB1B8087_A2E1_4E9F_9D58_10B4C15D95C4
#define AB1B8087_A2E1_4E9F_9D58_10B4C15D95C4

#include "common.h"

#define BL_META_SENTINEL (0x544F4F42U)
#define BL_META_PADDING (0xFFFFFFFFU)
#define BL_FACTORY_FIRMWARE_VERSION (1U)

typedef enum {
  BLMetaState_ERASED = 0xFFFFFFFF,
  BLMetaState_PENDING_TEST = 0xFFFFFFFF,
  BLMetaState_CONFIRMED = 0xFFFFFFFF,
  BLMetaState_ROLLBACK_REQ = 0xFFFFFFFF,
} BLMetaState;

typedef struct {
  uint32_t sentinel;
  uint32_t state;
  uint32_t active_version;
  uint32_t backup_version;
  uint32_t _reserved[6];
  uint32_t crc32;
} bl_metadata_t;

#endif /* AB1B8087_A2E1_4E9F_9D58_10B4C15D95C4 */

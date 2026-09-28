#include "bl_metadata.h"

// Contains the default bootloader metadata.

SECTION(".bootloader_metadata")
bl_metadata_t bl_metadata = {
    .sentinel = BL_META_SENTINEL,
    .state = BLMetaState_CONFIRMED,
    .active_version = BL_FACTORY_FIRMWARE_VERSION,
    .backup_version = BL_FACTORY_FIRMWARE_VERSION,
    ._reserved =
        {
            BL_META_PADDING,
            BL_META_PADDING,
            BL_META_PADDING,
            BL_META_PADDING,
            BL_META_PADDING,
            BL_META_PADDING,
        },
    .crc32 = 0xC3C5C0CC,
};
#include "common.h"
#include "fw_info.h"

SECTION(".firmware_info")
firmware_info_t fw_info = {.sentinel = FW_INFO_SENTINEL,
                           .device_id = DEVICE_ID,
                           // Calculated upon transfer
                           .version = 0xffffffff,
                           .length = 0xffffffff,
                           ._reserved0 = 0xffffffff,
                           ._reserved1 = 0xffffffff,
                           ._reserved2 = 0xffffffff,
                           ._reserved3 = 0xffffffff,
                           ._reserved4 = 0xDEADC0DE,
                           .crc32 = 0xffffffff};

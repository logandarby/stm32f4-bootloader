#ifndef D6F4E048_F1A1_4AEA_9BF2_BD583770C2B8
#define D6F4E048_F1A1_4AEA_9BF2_BD583770C2B8

#include "common.h"

/**
 * Erases the current firmware loaded on the chip
 */
void bl_flash_erase_firmware(void);

/**
 * Writes data to an address in flash memory
 */
void bl_flash_write(const uint32_t addr, const uint8_t* data,
                    const size_t data_len);

#endif /* D6F4E048_F1A1_4AEA_9BF2_BD583770C2B8 */

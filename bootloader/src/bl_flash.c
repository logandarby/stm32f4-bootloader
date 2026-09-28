#include "bl_flash.h"

#include <libopencm3/stm32/flash.h>

#include "memory.h"

void bl_flash_write(const uint32_t addr, const uint8_t* data,
                    const size_t data_len) {
  flash_unlock();
  flash_program(addr, data, data_len);
  flash_lock();
}

void bl_flash_erase_firmware(void) {
  flash_unlock();
  for (size_t i = FIRMWARE_SECTOR_START; i <= FIRMWARE_SECTOR_END; i++) {
    flash_erase_sector(i, FLASH_CR_PROGRAM_X32);
  }
  flash_lock();
}

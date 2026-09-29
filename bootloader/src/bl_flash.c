#include "bl_flash.h"

#include <libopencm3/stm32/flash.h>

#include "memory.h"

void bl_flash_write(uint32_t addr, const uint8_t* data,
                    const size_t data_len) {
  flash_unlock();
  flash_program(addr, data, data_len);
  flash_lock();
}

void bl_flash_erase_metadata() {
  flash_unlock();
  flash_erase_sector(2, FLASH_CR_PROGRAM_X32);
  flash_lock();
}

void bl_flash_erase_firmware(const BLFlashFWSlot slot) {
  flash_unlock();
  switch (slot) {
    case BLFlashFWSlot_A:
      flash_erase_sector(FIRMWARE_SLOT_A_SECTOR, FLASH_CR_PROGRAM_X32);
      break;
    case BLFlashFWSlot_B:
      flash_erase_sector(FIRMWARE_SLOT_B_SECTOR, FLASH_CR_PROGRAM_X32);
      break;
    case BLFlashFWSlot_SCRATCH:
      flash_erase_sector(FIRMWARE_SLOT_SCRATCH_SECTOR,
                         FLASH_CR_PROGRAM_X32);
      break;
  }
  flash_lock();
}

#ifndef C1D176F4_2C9E_43B0_B6F0_CDB574FF5DD0
#define C1D176F4_2C9E_43B0_B6F0_CDB574FF5DD0

#include <libopencm3/stm32/memorymap.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "portability.h"

#define CPU_FREQ_HZ (84000000U)
#define BOOTLOADER_SIZE (0x8000U)
#define FLASH_SIZE (1024U * 512U)
#define FIRMWARE_START_ADDR (FLASH_BASE + BOOTLOADER_SIZE)

#endif /* C1D176F4_2C9E_43B0_B6F0_CDB574FF5DD0 */

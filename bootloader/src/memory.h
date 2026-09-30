#ifndef D6DAE33C_9CA0_486A_B1AB_1C54C9ABE070
#define D6DAE33C_9CA0_486A_B1AB_1C54C9ABE070

#include "common.h"

// Linker script memory markers
extern uint8_t _boot_metadata_start[];
extern uint8_t _boot_metadata_end[];

extern uint8_t _slot_a_start[];
extern uint8_t _slot_a_max_size[];

extern uint8_t _slot_b_start[];
extern uint8_t _slot_b_max_size[];

extern uint8_t _slot_scratch_start[];
extern uint8_t _slot_scratch_max_size[];

#define LD_BOOT_METADATA_START ((uintptr_t)_boot_metadata_start)
#define LD_SLOT_A_START ((uintptr_t)_slot_a_start)
#define LD_SLOT_A_MAX_SIZE ((uintptr_t)_slot_a_max_size)
#define LD_SLOT_B_START ((uintptr_t)_slot_b_start)
#define LD_SLOT_B_MAX_SIZE ((uintptr_t)_slot_b_max_size)
#define LD_SLOT_SCRATCH_START ((uintptr_t)_slot_scratch_start)
#define LD_SLOT_SCRATCH_MAX_SIZE ((uintptr_t)_slot_scratch_max_size)

#define FIRMWARE_MAX_LEN (MIN(LD_SLOT_A_MAX_SIZE, LD_SLOT_B_MAX_SIZE))
#define LD_MAX_SLOT_SIZE FIRMWARE_MAX_LEN

#define FW_ACTIVE_ADDR (LD_SLOT_A_START)
#define FW_ACTIVE_INFO_ADDR (FW_ACTIVE_ADDR + sizeof(vector_table_t))
#define FW_ACTIVE_INFO_VALIDATE_FROM \
  (FW_ACTIVE_INFO_ADDR + sizeof(firmware_info_t))

#define FW_STAGING_ADDR (LD_SLOT_B_START)
#define FW_STAGING_INFO_ADDR (FW_STAGING_ADDR + sizeof(vector_table_t))
#define FW_STAGING_INFO_VALIDATE_FROM \
  (FW_STAGING_INFO_ADDR + sizeof(firmware_info_t))

#define FIRMWARE_SLOT_A_SECTOR (5U)
#define FIRMWARE_SLOT_B_SECTOR (6U)
#define FIRMWARE_SLOT_SCRATCH_SECTOR (7U)

#endif /* D6DAE33C_9CA0_486A_B1AB_1C54C9ABE070 */

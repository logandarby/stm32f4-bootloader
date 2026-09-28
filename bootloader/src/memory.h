#ifndef D6DAE33C_9CA0_486A_B1AB_1C54C9ABE070
#define D6DAE33C_9CA0_486A_B1AB_1C54C9ABE070

#include "common.h"

// Linker script memory markers
extern uint32_t _boot_metadata_start;
extern uint32_t _boot_metadata_end;

extern uint32_t _slot_a_start;
extern uint32_t _slot_a_end;
extern uint32_t _slot_a_max_size;

extern uint32_t _slot_b_start;
extern uint32_t _slot_b_end;
extern uint32_t _slot_b_max_size;

#define LD_BOOT_METADATA_START ((uintptr_t)&_boot_metadata_start)
#define LD_BOOT_METADATA_END ((uintptr_t)&_boot_metadata_end)
#define LD_SLOT_A_START ((uintptr_t)&_slot_a_start)
#define LD_SLOT_A_END ((uintptr_t)&_slot_a_end)
#define LD_SLOT_A_MAX_SIZE ((uint32_t)&_slot_a_max_size)
#define LD_SLOT_B_START ((uintptr_t)&_slot_b_start)
#define LD_SLOT_B_END ((uintptr_t)&_slot_b_end)
#define LD_SLOT_B_MAX_SIZE ((uint32_t)&_slot_b_max_size)

#define FIRMWARE_START_ADDR (LD_SLOT_A_START)
#define FW_INFO_ADDR (FIRMWARE_START_ADDR + sizeof(vector_table_t))
#define FIRMWARE_MAX_LEN (MIN(LD_SLOT_A_MAX_SIZE, LD_SLOT_B_MAX_SIZE))
#define FW_INFO_VALIDATE_FROM (FW_INFO_ADDR + sizeof(firmware_info_t))

#define FIRMWARE_SECTOR_START (3U)
#define FIRMWARE_SECTOR_END (7U)

#endif /* D6DAE33C_9CA0_486A_B1AB_1C54C9ABE070 */

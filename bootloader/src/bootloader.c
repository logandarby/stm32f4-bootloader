#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/memorymap.h>

#include "common.h"

#define MAIN_APP_START_ADDR (FLASH_BASE + BOOTLOADER_SIZE)

NORETURN int main(void) {
  cm_mask_interrupts(1);
  SCB_VTOR = MAIN_APP_START_ADDR;
  vector_table_t* vector_table_l = (vector_table_t*)MAIN_APP_START_ADDR;
  // Set MSP Register
  __asm__ volatile(
      "msr msp, %0 \n"  // set the msp
      "isb \n"          // instr sync barrier
      :
      : "r"(vector_table_l->initial_sp_value)
      : "memory");
  // Setup vector table
  vector_table_l->reset();
  while (1) {
    // Reset should never return, but in case it does we spin
  };
}
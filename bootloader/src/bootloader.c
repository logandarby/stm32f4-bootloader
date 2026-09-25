#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/memorymap.h>

#include "common.h"
#include "firmware-transfer.h"
#include "packet-transfer.h"
#include "system.h"
#include "uart.h"

static NORETURN void jump_to_main(void) {
  // const uint32_t mask = cm_mask_interrupts(1);
  SCB_VTOR = FIRMWARE_START_ADDR;
  vector_table_t* vector_table_l = (vector_table_t*)FIRMWARE_START_ADDR;
  // Set MSP Register
  __asm__ volatile(
      "msr msp, %0 \n"  // set the msp
      "isb \n"          // instr sync barrier
      :
      : "r"(vector_table_l->initial_sp_value)
      : "memory");
  // (void)cm_mask_interrupts(mask);
  // Setup vector table
  vector_table_l->reset();
  while (1) {
    // Reset should never return, but in case it does we spin
  };
}

NORETURN int main(void) {
  // Test packet transfer
  system_setup();
  uart_setup();
  packet_setup();

  firmware_transfer();

  uart_teardown();
  system_teardown();

  jump_to_main();
}
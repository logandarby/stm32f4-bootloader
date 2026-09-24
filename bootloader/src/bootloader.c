#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/memorymap.h>

#include "common.h"
#include "packet-transfer.h"
#include "scheduler.h"
#include "system.h"
#include "uart.h"

#define MAIN_APP_START_ADDR (FLASH_BASE + BOOTLOADER_SIZE)

static NORETURN void jump_to_main(void) {
  // const uint32_t mask = cm_mask_interrupts(1);
  SCB_VTOR = MAIN_APP_START_ADDR;
  vector_table_t* vector_table_l = (vector_table_t*)MAIN_APP_START_ADDR;
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

static void handle_uart(void) {
  packet_t test = {
      .data = {'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!'},
  };
  packet_init(&test, 12);
  packet_send(&test);
}

static void read_packets(void) { packet_update(); }

NORETURN int main(void) {
  // Test packet transfer
  system_setup();
  uart_setup();
  packet_setup();
  handle_uart();
  task_t tasks[] = {
      {.callback = handle_uart, .interval_ms = 4000},
      {.callback = read_packets, .interval_ms = 10},
  };

  scheduler_run(tasks, sizeof(tasks) / sizeof(task_t));
  jump_to_main();
}
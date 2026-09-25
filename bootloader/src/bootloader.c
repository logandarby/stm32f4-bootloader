#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/memorymap.h>

#include "common.h"
#include "firmware-transfer.h"
#include "inbuilt-button.h"
#include "led.h"
#include "packet-transfer.h"
#include "system.h"
#include "uart.h"

static NORETURN void jump_to_main(void) {
  __asm__ volatile("cpsid i" : : : "memory");
  vector_table_t* target_vectors = (vector_table_t*)FIRMWARE_START_ADDR;
  uint32_t app_sp = (uint32_t)target_vectors->initial_sp_value;
  void (*app_reset_handler)(void) = target_vectors->reset;
  __asm__ volatile(
      "msr msp, %0 \n"
      "isb         \n"
      "bx  %1      \n"
      :
      : "r"(app_sp), "r"(app_reset_handler)
      : "memory");
  while (1) {
    // Spin lock fallback
  }
}

NORETURN int main(void) {
  // Test packet transfer
  system_setup();
  uart_setup();
  packet_setup();
  inbuilt_button_setup();
  led_setup();

  // Firmware transfer mode only initiates if button is pressed
  if (inbuilt_button_is_pressed()) {
    led_set(LedState_ON);
    firmware_transfer_start();
    led_set(LedState_OFF);
  }

  inbuilt_button_teardown();
  led_teardown();
  uart_teardown();
  system_teardown();

  jump_to_main();
}
#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/memorymap.h>

#include "common.h"
#include "crc.h"
#include "firmware-transfer.h"
#include "firmware_validation.h"
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
static volatile uint32_t crc_val = 0;
NORETURN int main(void) {
  // Test packet transfer
  system_setup();
  inbuilt_button_setup();

  // Firmware transfer mode only initiates if button is pressed
  if (inbuilt_button_is_pressed()) {
    uart_setup();
    packet_setup();
    led_setup();

    led_set(LedState_ON);
    firmware_transfer_start();
    led_set(LedState_OFF);

    led_teardown();
    uart_teardown();
  }

  crc32_setup();
  const bool is_firmware_valid = fw_validate_firmware_image();
  crc32_teardown();

  inbuilt_button_teardown();
  system_teardown();

  if (is_firmware_valid) {
    jump_to_main();
  } else {
    scb_reset_core();
  }
}
#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/mpu.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/stm32/memorymap.h>

#include "bl_metadata.h"
#include "common.h"
#include "crc.h"
#include "firmware_transfer.h"
#include "firmware_validation.h"
#include "inbuilt-button.h"
#include "led.h"
#include "memory.h"
#include "packet-transfer.h"
#include "system.h"
#include "uart.h"

static void mpu_protect_bootloader(void) {
  __asm__ volatile("dmb" : : : "memory");
  MPU_CTRL = 0;
  MPU_RNR = 0;            // Select Region 0
  MPU_RBAR = FLASH_BASE;  // Base address matches bootloader start

  uint32_t rasr = 0;
  rasr |= (1 << 0);   // Enable region
  rasr |= (14 << 1);  // Size: 32 KB -> N = 14 (2^(14+1) = 32768 bytes)
  rasr |= (1 << 16);  // B (Bufferable) - Normal memory type
  rasr |= (1 << 17);  // C (Cacheable)  - Normal memory type
  // TEX is 0 (bits [21:19])
  rasr |= (5 << 24);  // AP: Read-Only for Privileged/Unprivileged (binary
                      // 101 at bits [26:24])
  rasr |= (0 << 28);  // XN: Execute allowed (0)

  MPU_RASR = rasr;
  MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;
  __asm__ volatile("dsb; isb" : : : "memory");
}

static NORETURN void jump_to_main(void) {
  __asm__ volatile("cpsid i" : : : "memory");
  // mpu_protect_bootloader();
  SCB_VTOR = LD_SLOT_A_START;
  vector_table_t* target_vectors = (vector_table_t*)LD_SLOT_A_START;
  target_vectors->reset();
  while (1) {
  }
  uint32_t app_sp = (uint32_t)target_vectors->initial_sp_value;
  void (*app_reset_handler)(void) = target_vectors->reset;
  __asm__ volatile(
      "msr msp, %0 \n"
      "cpsie i     \n"
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

    // We can skip crc since we already know the image is valid
    goto cleanup;
  }

  crc32_setup();
  const bool is_firmware_valid = fw_validate_firmware_image();
  crc32_teardown();

cleanup:

  inbuilt_button_teardown();
  system_teardown();

  if (is_firmware_valid) {
    jump_to_main();
  } else {
    scb_reset_core();
  }
}
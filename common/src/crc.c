#include "crc.h"

#include <libopencm3/stm32/crc.h>
#include <libopencm3/stm32/rcc.h>

uint8_t crc8(const uint8_t* data, size_t len) {
  uint8_t crc = 0x00;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x80) {
        crc = (uint8_t)((crc << 1) ^ 0x07);
      } else {
        crc = (uint8_t)(crc << 1);
      }
    }
  }
  return crc;
}

void crc32_setup(void) {
  rcc_periph_clock_enable(RCC_CRC);
  crc_reset();
}

void crc32_reset(void) { crc_reset(); }

uint32_t crc32_update(uint32_t data) { return crc_calculate(data); }

uint32_t crc32_update_block(const uint32_t* data, size_t len) {
  // SAFETY: This function does not modify the data pointer
  return crc_calculate_block((uint32_t*)data, len);
}

uint32_t crc32_final(void) { return CRC_DR; }

void crc32_teardown(void) { rcc_periph_clock_disable(RCC_CRC); }

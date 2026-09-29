#include "iwdg.h"

#include <libopencm3/stm32/iwdg.h>
#include <libopencm3/stm32/rcc.h>

void iwdg_set_countdown(void) {
  iwdg_set_period_ms(IWDG_COUNTDOWN_S * 1000);
  iwdg_start();
}

void iwdg_ack(void) { iwdg_reset(); }

bool iwdg_was_watchdog_reset(void) {
  return (RCC_CSR & RCC_CSR_IWDGRSTF) != 0;
}

void iwdg_clear_reset_flag(void) { RCC_CSR |= RCC_CSR_RMVF; }

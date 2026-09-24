#include "system.h"

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

// SAFETY: Should never be written to, with exception of sys_tick_handler.
// When read, should either be in a critical section, or use some other
// technique to avoid torn reads
static volatile uint64_t system_ms = 0;

void sys_tick_handler(void) {
  // SAFETY: Only one writer exists, torn writes cannot happen
  system_ms++;
}

static void setup_systick(void) {
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_set_frequency(SYS_TICK_FREQ_HZ, CPU_FREQ_HZ);
  systick_counter_enable();
  systick_interrupt_enable();
  cm_enable_interrupts();
}

static void clock_setup(void) {
  /* Use HSI (Internal 16 MHz RC Oscillator) scaled up to 84 MHz via PLL.
   * This avoids deadlocking on missing external crystal hardware. */
  rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

void system_enable(void) {
  clock_setup();
  setup_systick();
}

uint64_t system_get_ms(void) {
  uint32_t mask = cm_mask_interrupts(1);
  uint64_t ticks = system_ms;
  (void)cm_mask_interrupts(mask);
  return ticks;
}
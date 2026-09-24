#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "scheduler.h"
#include "system.h"

#define LED_PORT (GPIOA)
#define LED_PIN (GPIO5)
#define LED_RCC (RCC_GPIOA)

static void gpio_setup(void) {
  rcc_periph_clock_enable(LED_RCC);
  gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
}

static void toggle_led(void) { gpio_toggle(LED_PORT, LED_PIN); }

NORETURN int main(void) {
  system_setup();
  gpio_setup();

  task_t tasks[] = {
      {.callback = toggle_led, .interval_ms = 1000},
  };

  scheduler_run(tasks, sizeof(tasks) / sizeof(task_t));
}
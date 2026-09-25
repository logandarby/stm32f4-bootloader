#include "led.h"

void led_setup() {
  rcc_periph_clock_enable(LED_RCC);
  gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
}

void led_toggle() { gpio_toggle(LED_PORT, LED_PIN); }

void led_set(LedState set) {
  if (set == LedState_ON) {
    gpio_set(LED_PORT, LED_PIN);
  } else {
    gpio_clear(LED_PORT, LED_PIN);
  }
}

void led_teardown() {
  rcc_periph_clock_disable(LED_RCC);
  gpio_mode_setup(LED_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, LED_PIN);
}

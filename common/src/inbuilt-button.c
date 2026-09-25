#include "inbuilt-button.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

void inbuilt_button_setup(void) {
  rcc_periph_clock_enable(INBUILT_BUTTON_RCC);
  gpio_mode_setup(INBUILT_BUTTON_PORT, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
                  INBUILT_BUTTON_PIN);
}

void inbuilt_button_teardown(void) {
  rcc_periph_clock_disable(INBUILT_BUTTON_RCC);
  gpio_mode_setup(INBUILT_BUTTON_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  INBUILT_BUTTON_PIN);
}

bool inbuilt_button_is_pressed(void) {
  return (gpio_get(INBUILT_BUTTON_PORT, INBUILT_BUTTON_PIN) &
          INBUILT_BUTTON_PIN) == 0;
}
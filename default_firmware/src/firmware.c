#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "iwdg.h"
#include "led.h"
#include "scheduler.h"
#include "system.h"

NORETURN int main(void) {
  system_setup();
  led_setup();

  task_t tasks[] = {
      {.callback = led_toggle, .interval_ms = 1000},
      {.callback = iwdg_ack,
       .interval_ms = IWDG_RECOMMENDED_ACK_INTERVAL_S},

  };

  scheduler_run(tasks, sizeof(tasks) / sizeof(task_t));
  while (1) {
    __asm__("nop");
  }
}
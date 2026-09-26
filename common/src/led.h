#ifndef A3B63FAB_7D0C_4BF7_80A6_792926AF43C6
#define A3B63FAB_7D0C_4BF7_80A6_792926AF43C6

/**
 * Simple interface for the built-in LED
 */

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "common.h"

#define LED_PORT (GPIOA)
#define LED_PIN (GPIO5)
#define LED_RCC (RCC_GPIOA)

typedef enum {
  LedState_ON,
  LedState_OFF,
} LedState;

/**
 * Setsup the LED on the board
 * Uses the GPIO port and pins at the top of this file. Requires these
 * aren't being used for anything else, and aren't configured for any
 * alternate functions
 * NOTE: Requires you call led_teardown at the end of the program
 */
void led_setup(void);

void led_toggle(void);

void led_set(LedState set);

void led_teardown(void);

#endif /* A3B63FAB_7D0C_4BF7_80A6_792926AF43C6 */

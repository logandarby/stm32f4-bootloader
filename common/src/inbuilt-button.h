#ifndef DC93BD3E_55B5_42FC_8C56_E6AE5FACCAF7
#define DC93BD3E_55B5_42FC_8C56_E6AE5FACCAF7

/**
 * File to use the blue pushbutton on the board of the STM32F4
 */

#include "common.h"

#define INBUILT_BUTTON_PORT (GPIOC)
#define INBUILT_BUTTON_PIN (GPIO13)
#define INBUILT_BUTTON_RCC (RCC_GPIOC)

/**
 * Sets up PC13 to accept the button.
 * make sure this isn't set to any alternate functions (AF) or other uses
 * NOTE: Requires you call teardown after
 */
void inbuilt_button_setup(void);

void inbuilt_button_teardown(void);

bool inbuilt_button_is_pressed(void);

#endif /* DC93BD3E_55B5_42FC_8C56_E6AE5FACCAF7 */

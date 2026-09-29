#ifndef DE36CD95_84F5_452F_94BF_9C70D6BA7A3D
#define DE36CD95_84F5_452F_94BF_9C70D6BA7A3D

/**
 * This file handles the Independent Watchdog, who is set upon firmware
 * transfer success, and it is up to the firmware to acknowledge its
 * working by reset it, else an interrupt is triggered.
 */

#include "common.h"

#define IWDG_COUNTDOWN_S (5U)
#define IWDG_RECOMMENDED_ACK_INTERVAL_S (2U)

/**
 * For the bootloader to set the countdown
 */
void iwdg_set_countdown(void);

/**
 * For the firmware to acknowledge that it's working at a regular intercal
 */
void iwdg_ack(void);

bool iwdg_was_watchdog_reset(void);

void iwdg_clear_reset_flag(void);

#endif /* DE36CD95_84F5_452F_94BF_9C70D6BA7A3D */

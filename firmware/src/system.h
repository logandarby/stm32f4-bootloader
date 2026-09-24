#ifndef BC875EB7_F88A_4063_8E40_26D495BDC53F
#define BC875EB7_F88A_4063_8E40_26D495BDC53F

#include "common.h"

#define SYS_TICK_FREQ_HZ (1000)

/**
 * Startup the system. Enable clock and timers
 */
void system_enable(void);

/**
 * Get the current system ticks, measured in milliseconds
 */
uint64_t system_get_ms(void);

// DO NOT Call this
void sys_tick_handler(void);

#endif /* BC875EB7_F88A_4063_8E40_26D495BDC53F */

#ifndef D9790484_D6CE_426E_9055_E6C40910F396
#define D9790484_D6CE_426E_9055_E6C40910F396

#include "common.h"

/**
 * Validates a firmware image, using its embedded firmware_info_t struct.
 * If this struct doesn't exist or is corrupted, then returns false.
 *
 * This should run before every start to ensure that the firmware isn't
 * corrupted
 *
 * REQUIRES: The CRC module is engaged
 */
bool fw_validate_active_firmware_image(void);

/**
 * Validates the staged firmware (in slot B)
 *
 * REQUIRES: The CRC module is engaged
 */
bool fw_validate_staged_firmware_image(void);

#endif /* D9790484_D6CE_426E_9055_E6C40910F396 */

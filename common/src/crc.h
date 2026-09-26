#ifndef A60CBAAB_1D4F_4FA3_A7CD_68E010357B34
#define A60CBAAB_1D4F_4FA3_A7CD_68E010357B34

#include "common.h"

/**
 * Standalone CRC 8 implementation.
 */
uint8_t crc8(const uint8_t* data, size_t length);

/**
 * Initialize CRC Peripheral to use CRC32
 * Must be called before crc32_update().
 *
 * CRC32 uses ethernet standard polynomial 0x4C11DB7
 */
void crc32_setup(void);

/**
 * Reset the CRC calculation to its initial state.
 */
void crc32_reset(void);

/**
 * Feed data into the CRC calculator.
 */
uint32_t crc32_update_block(const uint32_t* data, size_t len);

/**
 * Feed data into the CRC calculator.
 */
uint32_t crc32_update(uint32_t data);

/**
 * Return the current CRC value.
 */
uint32_t crc32_final(void);

/**
 * Disable the CRC peripheral clock.
 *
 * Call after the CRC calculation is complete if the peripheral
 * is no longer needed.
 */
void crc32_teardown(void);

#endif /* A60CBAAB_1D4F_4FA3_A7CD_68E010357B34 */

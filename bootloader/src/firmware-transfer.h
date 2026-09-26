#ifndef BFDFD9C4_F3DC_4464_8255_CF8F65B127F8
#define BFDFD9C4_F3DC_4464_8255_CF8F65B127F8

/**
 * This file implements the firmware transfer protocol as defined in
 * DOCS.md
 */

#include "common.h"

// Constants
#define FW_DEFAULT_TIMEOUT_MS (3000U)
#define FW_MAX_FW_LEN (FLASH_SIZE - BOOTLOADER_SIZE)

// Sentinel Bytes for comms
#define FW_BYTE_SEQ_OBSERVED (0xA1U)
#define FW_BYTE_UPDATE_REQ (0xA2U)
#define FW_BYTE_UPDATE_RES (0xA3U)
#define FW_BYTE_DEVICE_ID_REQ (0xA4U)
#define FW_BYTE_DEVICE_ID_RES (0xA5U)
#define FW_BYTE_FW_LEN_REQ (0xA6U)
#define FW_BYTE_FW_LEN_RES (0xA7U)
#define FW_BYTE_READY (0xA8U)
#define FW_BYTE_FW_UPDATE_SUCCESSFUL (0xA9U)
#define FW_BYTE_NACK (0xABU)

// Sentinel values for the sync sequence

#define SYNC_SEQ_0 (0xC1)
#define SYNC_SEQ_1 (0xC3)
#define SYNC_SEQ_2 (0xC5)
#define SYNC_SEQ_3 (0xC7)

/**
 * Initializes a firmware transfer as defined in DOCS
 * Idles and blocks the application until a sync sequence is sent. Then the
 * firmware transfer sequence is initiated, and times out after value
 * FW_DEFAULT_TIMEOUT
 */
void firmware_transfer_start(void);

#endif /* BFDFD9C4_F3DC_4464_8255_CF8F65B127F8 */

#ifndef D271DC85_4118_40FA_BA85_847B5679EB52
#define D271DC85_4118_40FA_BA85_847B5679EB52

/**
 * Protocol built on top of UART to transfer data packets between host and
 * hardware Documentation in DOCS.md
 */

#include "common.h"

#define PACKET_BYTE_ACK (0x15U)
#define PACKET_BYTE_RETX (0x19U)
#define PACKET_BYTE_PADDING (0xFFU)

#define PACKET_DATA_LEN (16U)
#define PACKET_LENGTH_BYTES (1U)
#define PACKET_CRC_BYTES (1U)
#define PACKET_TOTAL_BYTES \
  (PACKET_DATA_LEN + PACKET_LENGTH_BYTES + PACKET_CRC_BYTES)

PACKED_STRUCT_BEGIN
typedef struct {
  uint8_t length;
  uint8_t data[PACKET_DATA_LEN];
  uint8_t crc;
} packet_t;
PACKED_STRUCT_END

/**
 * Sets up for packet recieving
 * Requires: uart_setup() has been called
 */
void packet_setup(void);

/**
 * Updates the packet state machine. Call at a poll rate
 */
void packet_update(void);

/**
 * Returns if there is a full packet available
 */
bool packet_is_data_available(void);

/**
 * If a packet is available, reads it into the buffer
 * Returns if it was successful or not
 */
bool packet_read(packet_t* packet_buffer);

/**
 * Sends the packet and returns if successful.
 * Make sure the CRC is computed correctly on the packet.
 */
bool packet_send(const packet_t* packet);

/**
 * Computes the CRC for a packet to send
 * Does not modify the packet
 */
uint8_t packet_compute_crc(const packet_t* packet);

#endif /* D271DC85_4118_40FA_BA85_847B5679EB52 */

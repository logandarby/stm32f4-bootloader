#ifndef D271DC85_4118_40FA_BA85_847B5679EB52
#define D271DC85_4118_40FA_BA85_847B5679EB52

/**
 * Protocol built on top of UART to transfer data packets between host and
 * hardware. Features 1-byte SOF alignment and a 3-retransmit limit to
 * prevent deadlock loops.
 */

#include "common.h"

#define PACKET_SOF_BYTE (0xAAU)

/* Control nibbles (upper 4 bits of the header byte) */
#define PACKET_CTRL_NONE (0x00U)
#define PACKET_CTRL_ACK (0x10U)
#define PACKET_CTRL_RETX (0x20U)
#define PACKET_CTRL_MASK (0xF0U)
#define PACKET_LEN_MASK (0x0FU)

#define PACKET_BYTE_PADDING (0xFFU)

#define PACKET_MAX_RETX_ATTEMPTS (3U)

#define PACKET_DATA_LEN (16U)
#define PACKET_SOF_BYTES (1U)
#define PACKET_HEADER_BYTES (1U)
#define PACKET_CRC_BYTES (1U)
#define PACKET_TOTAL_BYTES                                    \
  (PACKET_SOF_BYTES + PACKET_HEADER_BYTES + PACKET_DATA_LEN + \
   PACKET_CRC_BYTES)

PACKED_STRUCT_BEGIN
typedef struct {
  uint8_t sof; /* Always 0xAA */
  uint8_t
      header; /* Upper 4 bits: Control flags | Lower 4 bits: Length - 1 */
  uint8_t data[PACKET_DATA_LEN];
  uint8_t crc;
} packet_t;
PACKED_STRUCT_END

typedef enum {
  PacketSpecialType_RETX = PACKET_CTRL_RETX,
  PacketSpecialType_ACK = PACKET_CTRL_ACK,
} PacketSpecialType;

/**
 * Sets up for packet receiving
 * Requires: uart_setup() has been called
 */
void packet_setup(void);

/**
 * Takes a packet with data inside, and the data length, and initializes
 * all fields to be valid. NOTE: Assumes the package isn't a sentinel
 * packet like ACK or RETX
 */
void packet_init(packet_t* packet, size_t data_len);

/**
 * Creates a valid packet with a single byte payload as specified in byte
 * Copies the packet into `packet`
 */
void packet_create_single_byte(packet_t* packet, uint8_t byte);

/**
 * Checks if the packet contains a single byte, which is `byte`
 */
bool packet_is_single_byte(const packet_t* packet, uint8_t byte);

/**
 * Updates the packet state machine. Call at a poll rate.
 */
void packet_update(void);

/**
 * Returns if there is a full packet available.
 */
bool packet_is_data_available(void);

/**
 * If a packet is available, reads it into the buffer.
 * Returns if it was successful or not.
 */
bool packet_read(packet_t* packet_buffer);

/**
 * Sends the packet and returns if successful.
 * Calculates CRC automatically.
 */
bool packet_send(const packet_t* packet);

/**
 * Computes CRC-8 over the header and data array (excludes SOF and CRC
 * fields).
 */
uint8_t packet_compute_crc(const packet_t* packet);

/**
 * Get the length of the data inside the packet in bytes
 */
uint8_t packet_get_data_length(const packet_t* packet);

/**
 * Get the ctrl bits of the packet
 */
uint8_t packet_get_ctrl(const packet_t* packet);

/**
 * Helper to extract actual payload length (1..16) from packet header.
 */
static inline uint8_t packet_get_payload_len(const packet_t* packet) {
  return (packet->header & PACKET_LEN_MASK) + 1U;
}

#endif /* D271DC85_4118_40FA_BA85_847B5679EB52 */
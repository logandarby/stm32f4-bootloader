#include "packet-transfer.h"

#include <string.h>

#include "crc.h"
#include "ring_buffer.h"
#include "uart.h"

typedef enum {
  PacketState_SOF,
  PacketState_HEADER,
  PacketState_DATA,
  PacketState_CRC,
} PacketState;

static PacketState packet_state = PacketState_SOF;
static size_t data_byte_count = 0;
static packet_t temp_packet = {0};  // Builds current incoming packet
static packet_t packet_last_transmitted = {0};  // Cache for ReTx

static uint8_t retx_counter =
    0;  // Tracks consecutive ReTx requests sent/received

// Ring Buffer for received packets
#define RING_BUFFER_SIZE (16U)
static packet_t _packet_buffer_data[RING_BUFFER_SIZE];
static ringbuffer_t packet_ringbuffer = {0};

// Pre-configured Special Packets
static packet_t PACKET_RETX = {0};
static packet_t PACKET_ACK = {0};

static bool packet_is_ack(const packet_t* p) {
  return p && ((p->header & PACKET_CTRL_MASK) == PACKET_CTRL_ACK);
}

static bool packet_is_retx(const packet_t* p) {
  return p && ((p->header & PACKET_CTRL_MASK) == PACKET_CTRL_RETX);
}

static void packet_reset_rx_state(void) {
  packet_state = PacketState_SOF;
  data_byte_count = 0;
}

void packet_init(packet_t* packet, size_t data_len) {
  packet->sof = PACKET_SOF_BYTE;
  packet->header = PACKET_LEN_MASK & (data_len - 1);
  for (size_t i = data_len; i < PACKET_DATA_LEN; i++) {
    packet->data[i] = PACKET_BYTE_PADDING;
  }
  packet->crc = packet_compute_crc(packet);
}

void packet_setup(void) {
  packet_ringbuffer = rb_init(&_packet_buffer_data, RING_BUFFER_SIZE,
                              sizeof(_packet_buffer_data[0]));

  retx_counter = 0;

  // Initialize special packet structures
  PACKET_RETX.sof = PACKET_SOF_BYTE;
  PACKET_RETX.header = PACKET_CTRL_RETX;

  PACKET_ACK.sof = PACKET_SOF_BYTE;
  PACKET_ACK.header = PACKET_CTRL_ACK;

  for (size_t i = 0; i < PACKET_DATA_LEN; i++) {
    PACKET_RETX.data[i] = PACKET_BYTE_PADDING;
    PACKET_ACK.data[i] = PACKET_BYTE_PADDING;
  }

  PACKET_RETX.crc = packet_compute_crc(&PACKET_RETX);
  PACKET_ACK.crc = packet_compute_crc(&PACKET_ACK);

  packet_reset_rx_state();
}

void packet_update(void) {
  uint8_t byte_val;
  while (uart_read_byte(&byte_val)) {
    switch (packet_state) {
      case PacketState_SOF:
        if (byte_val == PACKET_SOF_BYTE) {
          temp_packet.sof = byte_val;
          packet_state = PacketState_HEADER;
        }
        break;

      case PacketState_HEADER:
        temp_packet.header = byte_val;
        data_byte_count = 0;
        packet_state = PacketState_DATA;
        break;

      case PacketState_DATA:
        temp_packet.data[data_byte_count++] = byte_val;
        if (data_byte_count == PACKET_DATA_LEN) {
          packet_state = PacketState_CRC;
        }
        break;

      case PacketState_CRC:
        temp_packet.crc = byte_val;
        uint8_t computed_crc = packet_compute_crc(&temp_packet);

        if (computed_crc != temp_packet.crc) {
          // CRC mismatch: Request ReTx if cap not reached
          retx_counter++;
          if (retx_counter <= PACKET_MAX_RETX_ATTEMPTS) {
            packet_send(&PACKET_RETX);
          } else {
            // Cap reached: Abort and reset protocol state
            retx_counter = 0;
          }
          packet_reset_rx_state();
          break;
        }

        // --- CRC Passed ---
        if (packet_is_retx(&temp_packet)) {
          // Received a ReTx request from host
          retx_counter++;
          if (retx_counter <= PACKET_MAX_RETX_ATTEMPTS) {
            packet_send(&packet_last_transmitted);
          } else {
            // Cap reached: Stop retransmitting and abort
            retx_counter = 0;
          }
          packet_reset_rx_state();
          break;
        }

        // Reset ReTx counter on successful valid transaction
        retx_counter = 0;

        if (packet_is_ack(&temp_packet)) {
          packet_reset_rx_state();
          break;
        }

        // Valid data packet received: Store in buffer & respond with ACK
        rb_write(&packet_ringbuffer, &temp_packet);
        packet_send(&PACKET_ACK);
        packet_reset_rx_state();
        break;

      default:
        packet_reset_rx_state();
        break;
    }
  }
}

bool packet_is_data_available(void) {
  return !rb_is_empty(&packet_ringbuffer);
}

bool packet_read(packet_t* packet_buffer) {
  return rb_read(&packet_ringbuffer, packet_buffer);
}

bool packet_send(const packet_t* packet) {
  packet_t tx_pkt;
  memcpy(&tx_pkt, packet, sizeof(packet_t));

  tx_pkt.sof = PACKET_SOF_BYTE;
  tx_pkt.crc = packet_compute_crc(&tx_pkt);

  // Cache non-special packets for retransmission
  if (!packet_is_ack(&tx_pkt) && !packet_is_retx(&tx_pkt)) {
    memcpy(&packet_last_transmitted, &tx_pkt, sizeof(packet_t));
  }

  return uart_send((const uint8_t*)&tx_pkt, PACKET_TOTAL_BYTES) ==
         PACKET_TOTAL_BYTES;
}

uint8_t packet_compute_crc(const packet_t* packet) {
  // Compute CRC over header (1 byte) + data (16 bytes)
  uint8_t header_and_data[PACKET_HEADER_BYTES + PACKET_DATA_LEN];
  header_and_data[0] = packet->header;
  memcpy(&header_and_data[1], packet->data, PACKET_DATA_LEN);

  return crc8(header_and_data, sizeof(header_and_data));
}
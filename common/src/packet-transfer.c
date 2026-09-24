#include "packet-transfer.h"

#include <string.h>

#include "crc.h"
#include "ring_buffer.h"
#include "uart.h"

typedef enum {
  PacketState_LENGTH,
  PacketState_DATA,
  PacketState_CRC,
} PacketState;

static PacketState packet_state = PacketState_LENGTH;
static size_t data_byte_count = 0;
static packet_t temp_packet = {0};  // Used to slowly build current packet
static packet_t packet_last_transmitted = {0};  // Used for retx

// Ring Buffer for recieved packets
#define RING_BUFFER_SIZE (16U)
static packet_t _packet_buffer_data[RING_BUFFER_SIZE];
static ringbuffer_t packet_ringbuffer = {0};

// Special Packets
static packet_t PACKET_RETX = {0};
static packet_t PACKET_ACK = {0};

static bool _packet_eq_helper(const packet_t* p,
                              const uint8_t special_byte) {
  if (!p || p->length != 1 || p->data[0] != special_byte) {
    return false;
  }
  for (size_t i = 1; i < PACKET_DATA_LEN; i++) {
    if (p->data[i] != PACKET_BYTE_PADDING) {
      return false;
    }
  }
  return true;
}

static bool packet_is_ack(const packet_t* p) {
  return _packet_eq_helper(p, PACKET_BYTE_ACK);
}

static bool packet_is_retx(const packet_t* p) {
  return _packet_eq_helper(p, PACKET_BYTE_RETX);
}

void packet_setup(void) {
  // Setup ring buffer
  packet_ringbuffer = rb_init(&_packet_buffer_data, RING_BUFFER_SIZE,
                              sizeof(_packet_buffer_data[0]));

  // Initialize special packets
  PACKET_RETX.length = 1;
  PACKET_ACK.length = 1;
  PACKET_RETX.data[0] = PACKET_BYTE_RETX;
  PACKET_ACK.data[0] = PACKET_BYTE_ACK;
  for (size_t i = 1; i < PACKET_DATA_LEN; i++) {
    PACKET_RETX.data[i] = PACKET_BYTE_PADDING;
    PACKET_ACK.data[i] = PACKET_BYTE_PADDING;
  }
  PACKET_RETX.crc = packet_compute_crc(&PACKET_RETX);
  PACKET_ACK.crc = packet_compute_crc(&PACKET_ACK);
}

void packet_update(void) {
  while (uart_is_data_available()) {
    switch (packet_state) {
      case PacketState_LENGTH:
        if (uart_read_byte(&temp_packet.length)) {
          packet_state = PacketState_DATA;
        }
        break;
      case PacketState_DATA:
        if (!uart_read_byte(&temp_packet.data[data_byte_count])) break;
        data_byte_count++;
        if (data_byte_count == PACKET_DATA_LEN) {
          data_byte_count = 0;
          packet_state = PacketState_CRC;
        }
        break;
      case PacketState_CRC:
        if (!uart_read_byte(&temp_packet.crc)) break;
        uint8_t packet_crc = packet_compute_crc(&temp_packet);
        if (packet_crc != temp_packet.crc) {
          packet_send(&PACKET_RETX);
          packet_state = PacketState_LENGTH;
          break;
        } else if (packet_is_retx(&temp_packet)) {
          packet_send(&packet_last_transmitted);
          packet_state = PacketState_LENGTH;
          break;
        } else if (packet_is_ack(&temp_packet)) {
          packet_state = PacketState_LENGTH;
          break;
        }
        // Packet is fine. Store in buffer and send ack
        rb_write(&packet_ringbuffer, &temp_packet);
        packet_send(&PACKET_ACK);
        packet_state = PacketState_LENGTH;
        break;
      default:
        break;
    }
  }
}

bool packet_is_data_available() {
  return !rb_is_empty(&packet_ringbuffer);
}

bool packet_read(packet_t* packet_buffer) {
  return rb_read(&packet_ringbuffer, packet_buffer);
}

bool packet_send(const packet_t* packet) {
  memcpy(&packet_last_transmitted, packet, sizeof(*packet));
  return uart_send((const uint8_t*)packet, PACKET_TOTAL_BYTES) ==
         PACKET_TOTAL_BYTES;
}

uint8_t packet_compute_crc(const packet_t* packet) {
  return crc8((uint8_t*)packet, PACKET_TOTAL_BYTES - PACKET_CRC_BYTES);
}
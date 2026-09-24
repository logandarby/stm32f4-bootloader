#include "ring_buffer.h"

#include "util.h"

static const ringbuffer_t EMPTY_RINGBUFFER = {0};

ringbuffer_t rb_init(uint8_t* buffer, size_t buffer_size) {
  if (!buffer || !buffer_size || !is_power_of_2(buffer_size)) {
    return EMPTY_RINGBUFFER;
  }
  const ringbuffer_t rb = {
      .buffer = buffer,
      .buffer_size = buffer_size,
      .mask = buffer_size - 1,
      .read_idx = 0,
      .write_idx = 0,
  };
  return rb;
}

bool rb_is_empty(const ringbuffer_t* rb) {
  return !rb || rb->read_idx == rb->write_idx;
}

bool rb_write_byte(ringbuffer_t* rb, const uint8_t byte) {
  if (!rb || !rb->buffer) {
    return false;
  }
  const size_t read_idx = rb->read_idx;
  const size_t write_idx = rb->write_idx;
  const size_t next_write_idx = (write_idx + 1) & rb->mask;
  if (next_write_idx == read_idx) {
    return false;
  }
  rb->buffer[write_idx] = byte;
  rb->write_idx = next_write_idx;
  return true;
}

bool rb_read_byte(ringbuffer_t* rb, uint8_t* byte_buffer) {
  if (!rb || !byte_buffer || !rb->buffer || rb_is_empty(rb)) {
    return false;
  }
  const size_t read_idx = rb->read_idx;
  *byte_buffer = rb->buffer[read_idx];
  const size_t next_read_idx = (read_idx + 1) & rb->mask;
  rb->read_idx = next_read_idx;
  return true;
}
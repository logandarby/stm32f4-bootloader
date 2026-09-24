#include "ring_buffer.h"

#include <string.h>

#include "util.h"

static const ringbuffer_t EMPTY_RINGBUFFER = {0};

ringbuffer_t rb_init(void* buffer, size_t buffer_size, size_t item_size) {
  // Validate that buffer_size is a power of 2 and item_size is valid
  if (!buffer || !buffer_size || !item_size ||
      !is_power_of_2(buffer_size)) {
    return EMPTY_RINGBUFFER;
  }
  const ringbuffer_t rb = {
      .buffer = (uint8_t*)buffer,
      .buffer_size = buffer_size,
      .item_size = item_size,
      .mask = buffer_size - 1,
      .read_idx = 0,
      .write_idx = 0,
  };
  return rb;
}

bool rb_is_empty(const ringbuffer_t* rb) {
  return !rb || rb->read_idx == rb->write_idx;
}

bool rb_write(ringbuffer_t* rb, const void* data) {
  if (!rb || !rb->buffer || !data) {
    return false;
  }
  const size_t read_idx = rb->read_idx;
  const size_t write_idx = rb->write_idx;
  const size_t next_write_idx = (write_idx + 1) & rb->mask;

  if (next_write_idx == read_idx) {
    return false;  // Buffer is full
  }

  // Calculate the byte offset and copy the generic data in
  size_t offset = write_idx * rb->item_size;
  memcpy(&rb->buffer[offset], data, rb->item_size);

  rb->write_idx = next_write_idx;
  return true;
}

bool rb_read(ringbuffer_t* rb, void* data) {
  if (!rb || !data || !rb->buffer || rb_is_empty(rb)) {
    return false;
  }
  const size_t read_idx = rb->read_idx;

  // Calculate the byte offset and copy the generic data out
  size_t offset = read_idx * rb->item_size;
  memcpy(data, &rb->buffer[offset], rb->item_size);

  const size_t next_read_idx = (read_idx + 1) & rb->mask;
  rb->read_idx = next_read_idx;
  return true;
}
#ifndef DC3FC35D_4447_46D1_9BD4_3F49DC3BA0C6
#define DC3FC35D_4447_46D1_9BD4_3F49DC3BA0C6

#include "common.h"

typedef struct {
  uint8_t* buffer;
  size_t buffer_size;
  uint32_t mask;
  volatile size_t read_idx;  // Volatile for interrupt safety
  volatile size_t write_idx;
} ringbuffer_t;

/**
 * Initializes a ring buffer and returns it.
 * The buffer size must be a power of 2
 * The ring buffer owns the buffer
 * SAFETY: This only works with SPSC code. It is not
 * thread-safe/multi-interrupt safe.
 */
ringbuffer_t rb_init(uint8_t* buffer, size_t buffer_size);

/**
 * Returns if the ring buffer is empty
 */
bool rb_is_empty(const ringbuffer_t* rb);

/**
 * Writes a byte to rb
 */
bool rb_write_byte(ringbuffer_t* rb, const uint8_t byte);

/**
 * Read a byte into byte_buffer
 * Returns if successfull
 */
bool rb_read_byte(ringbuffer_t* rb, uint8_t* byte_buffer);

#endif /* DC3FC35D_4447_46D1_9BD4_3F49DC3BA0C6 */

#ifndef DC3FC35D_4447_46D1_9BD4_3F49DC3BA0C6
#define DC3FC35D_4447_46D1_9BD4_3F49DC3BA0C6

#include "common.h"

typedef struct {
  uint8_t* buffer;
  size_t buffer_size;
  size_t item_size;  // Supports generic data types
  uint32_t mask;
  volatile size_t read_idx;  // Volatile for interrupt safety
  volatile size_t write_idx;
} ringbuffer_t;

/**
 * Initializes a ring buffer and returns it.
 * The buffer_size (number of elements) must be a power of 2.
 * The item size is the size of the items you wish to store
 * The backing buffer must be at least (buffer_size * item_size) bytes
 * large. SAFETY: This only works with SPSC code. It is not
 * thread-safe/multi-interrupt safe.
 */
ringbuffer_t rb_init(void* buffer, size_t buffer_size, size_t item_size);

/**
 * Returns if the ring buffer is empty
 */
bool rb_is_empty(const ringbuffer_t* rb);

/**
 * Writes an item to rb
 */
bool rb_write(ringbuffer_t* rb, const void* data);

/**
 * Read an item into data
 * Returns if successful
 */
bool rb_read(ringbuffer_t* rb, void* data);

#endif /* DC3FC35D_4447_46D1_9BD4_3F49DC3BA0C6 */
#ifndef FC230A32_595B_44C5_A8BB_3FAEC3948BB2
#define FC230A32_595B_44C5_A8BB_3FAEC3948BB2

#include <common.h>

/**
 * Setup UART
 * Requires system_setup() has been called
 */
void uart_setup(void);

bool uart_is_data_available(void);

void uart_send_byte(uint8_t byte);
size_t uart_send(const uint8_t* bytes, size_t bytes_len);

bool uart_read_byte(uint8_t* byte_buffer);
size_t uart_read(uint8_t* byte_buffer, size_t buffer_len);

#endif /* FC230A32_595B_44C5_A8BB_3FAEC3948BB2 */

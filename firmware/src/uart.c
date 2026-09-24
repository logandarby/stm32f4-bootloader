#include "uart.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include "ring_buffer.h"

#define BAUD_RATE (115200)
#define UART_PORT (GPIOA)
#define UART_PIN_TX (GPIO2)
#define UART_PIN_RX (GPIO3)
// SAFETY: Ring buffer of 128 gives about 10ms of latency
#define BUFFER_SIZE (128)

static ringbuffer_t uart_ringbuffer = {0};
static uint8_t _buffer[BUFFER_SIZE];

void usart2_isr(void) {
  const bool is_recv = usart_get_flag(USART2, USART_FLAG_RXNE);
  const bool is_overrun = usart_get_flag(USART2, USART_FLAG_ORE);
  if (is_recv || is_overrun) {
    uint8_t byte = (uint8_t)usart_recv(USART2);
    rb_write(&uart_ringbuffer, &byte);
  }
}

// We enable UART2 here, and set up the GPIO stuff
void uart_setup(void) {
  uart_ringbuffer = rb_init(_buffer, BUFFER_SIZE, sizeof(uint8_t));

  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_USART2);

  gpio_mode_setup(UART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  UART_PIN_TX | UART_PIN_RX);
  gpio_set_af(UART_PORT, GPIO_AF7, UART_PIN_TX | UART_PIN_RX);

  usart_set_mode(USART2, USART_MODE_TX_RX);
  usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
  usart_set_baudrate(USART2, BAUD_RATE);
  usart_set_databits(USART2, 8);
  usart_set_parity(USART2, 0);
  usart_set_stopbits(USART2, 1);

  usart_enable_rx_interrupt(USART2);
  nvic_enable_irq(NVIC_USART2_IRQ);

  usart_enable(USART2);
}

bool uart_is_data_available(void) {
  return !rb_is_empty(&uart_ringbuffer);
}

void uart_send_byte(uint8_t byte1) { usart_send_blocking(USART2, byte1); }

size_t uart_send(uint8_t* bytes, size_t bytes_len) {
  if (!bytes || !bytes_len) {
    return 0;
  }
  for (size_t i = 0; i < bytes_len; i++) {
    uart_send_byte(bytes[i]);
  }
  return bytes_len;
}

bool uart_read_byte(uint8_t* byte_buffer) {
  return uart_read(byte_buffer, 1) != 0;
}

size_t uart_read(uint8_t* byte_buffer, size_t buffer_len) {
  if (!byte_buffer || !buffer_len) {
    return 0;
  }
  for (size_t i = 0; i < buffer_len; i++) {
    if (!uart_is_data_available() ||
        !rb_read(&uart_ringbuffer, &byte_buffer[i])) {
      return i;
    }
  }
  return buffer_len;
}
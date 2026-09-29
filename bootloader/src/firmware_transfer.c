#include "firmware_transfer.h"

#include "bl_flash.h"
#include "firmware_validation.h"
#include "fw_info.h"
#include "memory.h"
#include "packet-transfer.h"
#include "timer.h"
#include "uart.h"

typedef enum {
  FWState_SYNC,
  FWState_WAIT_FOR_UPDATE_REQ,
  FWState_DEVICE_ID_REQ,
  FWState_DEVICE_ID_RES,
  FWState_FW_LEN_REQ,
  FWState_FW_LEN_RES,
  FWState_ERASE_FW,
  FWState_RECV_FW,
  FWState_DONE
} FWState;

static FWState fw_state = FWState_SYNC;
static uint32_t fw_length = 0;
static uint32_t fw_bytes_written = 0;
static uint8_t fw_sync_seq[4] = {0};
static packet_t fw_packet_buffer = {0};
static timer_t timeout = {0};
static bool is_failed = false;

static void fw_fail(void) {
  packet_create_single_byte(&fw_packet_buffer, FW_BYTE_NACK);
  (void)packet_send(&fw_packet_buffer);
  fw_state = FWState_DONE;
  is_failed = true;
}

static void check_timeout(void) {
  if (timer_has_elapsed(&timeout)) {
    fw_fail();
  }
}

static bool fw_is_device_id_packet(const packet_t* packet) {
  return packet_get_ctrl(packet) == PACKET_CTRL_NONE &&
         packet_get_data_length(packet) == 2 &&
         packet->data[0] == FW_BYTE_DEVICE_ID_RES &&
         packet->data[1] == DEVICE_ID;
}

static bool fw_is_fw_len_packet(const packet_t* packet) {
  return packet_get_ctrl(packet) == PACKET_CTRL_NONE &&
         packet_get_data_length(packet) == 5 &&
         packet->data[0] == FW_BYTE_FW_LEN_RES;
}

bool firmware_transfer_start(void) {
  is_failed = false;
  // Reset session variables
  fw_state = FWState_SYNC;
  fw_length = 0;
  fw_bytes_written = 0;

  timer_init(&timeout, FW_DEFAULT_TIMEOUT_MS, false);

  while (fw_state != FWState_DONE) {
    // Idle until sync observed
    if (fw_state != FWState_SYNC) {
      check_timeout();
    }

    if (fw_state == FWState_SYNC) {
      if (uart_is_data_available()) {
        fw_sync_seq[0] = fw_sync_seq[1];
        fw_sync_seq[1] = fw_sync_seq[2];
        fw_sync_seq[2] = fw_sync_seq[3];
        uart_read_byte(&fw_sync_seq[3]);

        if (fw_sync_seq[0] == SYNC_SEQ_0 && fw_sync_seq[1] == SYNC_SEQ_1 &&
            fw_sync_seq[2] == SYNC_SEQ_2 && fw_sync_seq[3] == SYNC_SEQ_3) {
          packet_create_single_byte(&fw_packet_buffer,
                                    FW_BYTE_SEQ_OBSERVED);
          (void)packet_send(&fw_packet_buffer);

          // Arm the timeout sequence upon completing handshakes
          timer_reset(&timeout);
          fw_state = FWState_WAIT_FOR_UPDATE_REQ;
        }
      }
      continue;
    }

    packet_update();

    switch (fw_state) {
      case FWState_WAIT_FOR_UPDATE_REQ: {
        if (!packet_is_data_available()) continue;
        timer_reset(&timeout);
        if (!packet_read(&fw_packet_buffer) ||
            !packet_is_single_byte(&fw_packet_buffer,
                                   FW_BYTE_UPDATE_REQ)) {
          fw_fail();
          continue;
        }
        packet_create_single_byte(&fw_packet_buffer, FW_BYTE_UPDATE_RES);
        (void)packet_send(&fw_packet_buffer);
        fw_state = FWState_DEVICE_ID_REQ;
      } break;

      case FWState_DEVICE_ID_REQ: {
        timer_reset(&timeout);
        packet_create_single_byte(&fw_packet_buffer,
                                  FW_BYTE_DEVICE_ID_REQ);
        (void)packet_send(&fw_packet_buffer);
        fw_state = FWState_DEVICE_ID_RES;
      } break;

      case FWState_DEVICE_ID_RES: {
        if (!packet_is_data_available()) continue;
        timer_reset(&timeout);
        if (!packet_read(&fw_packet_buffer) ||
            !fw_is_device_id_packet(&fw_packet_buffer)) {
          fw_fail();
          continue;
        }
        fw_state = FWState_FW_LEN_REQ;
      } break;

      case FWState_FW_LEN_REQ: {
        timer_reset(&timeout);
        packet_create_single_byte(&fw_packet_buffer, FW_BYTE_FW_LEN_REQ);
        (void)packet_send(&fw_packet_buffer);
        fw_state = FWState_FW_LEN_RES;
      } break;

      case FWState_FW_LEN_RES: {
        if (!packet_is_data_available()) continue;
        timer_reset(&timeout);

        if (!packet_read(&fw_packet_buffer) ||
            !fw_is_fw_len_packet(&fw_packet_buffer)) {
          fw_fail();
          continue;
        }

        fw_length = (uint32_t)fw_packet_buffer.data[1] |
                    ((uint32_t)fw_packet_buffer.data[2] << 8) |
                    ((uint32_t)fw_packet_buffer.data[3] << 16) |
                    ((uint32_t)fw_packet_buffer.data[4] << 24);

        // Enforce range check
        if (fw_length == 0 || fw_length > FIRMWARE_MAX_LEN) {
          fw_fail();
          continue;
        }

        fw_state = FWState_ERASE_FW;
      } break;

      case FWState_ERASE_FW: {
        timer_disable(&timeout);
        bl_flash_erase_firmware(BLFlashFWSlot_B);
        timer_enable(&timeout);
        timer_reset(&timeout);

        packet_create_single_byte(&fw_packet_buffer, FW_BYTE_READY);
        (void)packet_send(&fw_packet_buffer);

        fw_state = FWState_RECV_FW;
      } break;

      case FWState_RECV_FW: {
        if (!packet_is_data_available()) continue;
        timer_reset(&timeout);
        if (!packet_read(&fw_packet_buffer)) {
          fw_fail();
          continue;
        }

        uint32_t packet_len = packet_get_data_length(&fw_packet_buffer);
        uint32_t bytes_to_write = packet_len;

        // Prevent writing past the expected firmware length
        if (fw_bytes_written + bytes_to_write > fw_length) {
          bytes_to_write = fw_length - fw_bytes_written;
        }

        bl_flash_write(FW_STAGING_ADDR + fw_bytes_written,
                       fw_packet_buffer.data, bytes_to_write);

        fw_bytes_written += bytes_to_write;

        if (fw_bytes_written >= fw_length) {
          packet_create_single_byte(&fw_packet_buffer,
                                    FW_BYTE_FW_UPDATE_SUCCESSFUL);
          (void)packet_send(&fw_packet_buffer);
          fw_state = FWState_DONE;
        } else {
          packet_create_single_byte(&fw_packet_buffer, FW_BYTE_READY);
          (void)packet_send(&fw_packet_buffer);
        }
      } break;

      case FWState_DONE:
        if (!fw_validate_staged_firmware_image()) {
          fw_fail();
          continue;
        }
        continue;

      default:
        fw_state = FWState_DONE;
        break;
    }
  }

  // Make sure no other packets are currently sending
  uart_wait_for_tc();

  return !is_failed;
};

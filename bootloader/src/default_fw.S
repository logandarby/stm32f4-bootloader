    .syntax unified
    .cpu cortex-m4

    /* Embed into Slot A */
    .section .slot_a_data, "a", %progbits
    .global default_fw_slot_a
default_fw_slot_a:
    .incbin "../default_firmware/default_firmware.bin"
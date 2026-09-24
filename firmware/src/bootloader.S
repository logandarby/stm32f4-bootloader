.section .bootloader_section, "a"
.balign 4
.global bootloader_bin
bootloader_bin:
  .incbin "../bootloader/bootloader.bin"
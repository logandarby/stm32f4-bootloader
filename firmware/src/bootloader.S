.section .bootloader_section, "a"
.global bootloader_bin
bootloader_bin:
  .incbin "../bootloader/bootloader.bin"
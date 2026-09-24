BOOTLOADER_SIZE = 0x8000
BOOTLOADER_FILE = "../bootloader/bootloader.bin"

def main():
  with open(BOOTLOADER_FILE, "rb") as f:
    bootloader_file = f.read()
    
  bytes_to_pad = BOOTLOADER_SIZE - len(bootloader_file)
  padding = bytes([0xff for _ in range(bytes_to_pad)])
  
  with open(BOOTLOADER_FILE, "wb") as f:
    f.write(bootloader_file + padding)

if __name__ == "__main__":
  main()
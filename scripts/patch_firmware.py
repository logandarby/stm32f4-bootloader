import ctypes
import crcmod
import struct
import functools
import argparse

FW_INFO_SENTINEL = 0xC0FFEE00
DEVICE_ID = 0x14
VECTOR_TABLE_SIZE = 0x1AC  # sizeof(vector_table_t)
FW_INFO_OFFSET = VECTOR_TABLE_SIZE

# STM32 CRC-32 configuration:
# Poly: 0x104C11DB7, Init: 0xFFFFFFFF, Rev: False, XORout: 0x00000000
stm32_crc_func = crcmod.mkCrcFun(
    poly=0x104C11DB7, 
    initCrc=0xFFFFFFFF, 
    rev=False, 
    xorOut=0x00000000
)

# Mirror the C struct using Little-Endian uint32
class FirmwareInfo(ctypes.LittleEndianStructure):
    _fields_ = [
        ("sentinel", ctypes.c_uint32),
        ("device_id", ctypes.c_uint32),
        ("version", ctypes.c_uint32),
        ("length", ctypes.c_uint32),
        ("_reserved0", ctypes.c_uint32),
        ("_reserved1", ctypes.c_uint32),
        ("_reserved2", ctypes.c_uint32),
        ("_reserved3", ctypes.c_uint32),
        ("_reserved4", ctypes.c_uint32),
        ("crc32", ctypes.c_uint32),
    ]

    def __init__(self, **kwargs):
        # Set default 0xFFFFFFFF padding for all reserved fields
        for field in ("_reserved0", "_reserved1", "_reserved2", "_reserved3", "_reserved4"):
            kwargs.setdefault(field, 0xFFFFFFFF)
        super().__init__(**kwargs)


def stm32_crc32_fast(data: bytes) -> int:
    # Ensure byte padding to 32-bit (4-byte) alignment
    remainder = len(data) % 4
    if remainder != 0:
        data = data + b'\x00' * (4 - remainder)
    
    formatted_data = bytearray()
    for i in range(0, len(data), 4):
        # FIX: Unpack using Little-Endian ('<I') to match STM32 memory reads, 
        # then repack Big-Endian ('>I') for the MSB-first CRC engine.
        word = struct.unpack('<I', data[i:i+4])[0]
        formatted_data.extend(struct.pack('>I', word))

    return stm32_crc_func(bytes(formatted_data))

class FirmwarePatchError(Exception):
    """Base exception for all firmware patching errors."""
    pass

class FirmwareSizeError(FirmwarePatchError):
    """Raised when the firmware file is too small to contain required headers."""
    pass

class InvalidSentinelError(FirmwarePatchError):
    """Raised when the sentinel magic number does not match."""
    pass

class InvalidDeviceIdError(FirmwarePatchError):
    """Raised when the target device ID does not match."""
    pass

def patch_firmware(fw_data: bytearray, device_id: int, version: int) -> None:
    """Patches fw_data in-place with updated FirmwareInfo and calculated payload CRC32.
    
    Raises:
        FirmwareSizeError: If binary is smaller than vector table + header.
        InvalidSentinelError: If sentinel magic number does not match.
        InvalidDeviceIdError: If device ID does not match.
    """
    header_len = ctypes.sizeof(FirmwareInfo)
    payload_offset = FW_INFO_OFFSET + header_len
    # Bounds check
    if len(fw_data) < payload_offset:
        raise FirmwareSizeError(
            f"Firmware size ({len(fw_data)} bytes) is smaller than the required header "
            f"offset ({payload_offset} bytes)."
        )
    existing_header_bytes = fw_data[FW_INFO_OFFSET : FW_INFO_OFFSET + header_len]
    existing_info = FirmwareInfo.from_buffer_copy(existing_header_bytes)
    if existing_info.sentinel != FW_INFO_SENTINEL:
        raise InvalidSentinelError(
            f"Invalid sentinel: expected 0x{FW_INFO_SENTINEL:08X}, got 0x{existing_info.sentinel:08X}."
        )
    if existing_info.device_id != device_id:
        raise InvalidDeviceIdError(
            f"Invalid device ID: expected 0x{device_id:02X}, got 0x{existing_info.device_id:02X}."
        )
    payload = fw_data[payload_offset:]
     
    payload_crc = stm32_crc32_fast(bytes(payload))
    updated_info = FirmwareInfo(
        sentinel=existing_info.sentinel,
        device_id=existing_info.device_id,
        version=version,
        length=len(fw_data),
        _reserved0=existing_info._reserved0,
        _reserved1=existing_info._reserved1,
        _reserved2=existing_info._reserved2,
        _reserved3=existing_info._reserved3,
        _reserved4=existing_info._reserved4,
        crc32=payload_crc,
    )
    fw_data[FW_INFO_OFFSET : FW_INFO_OFFSET + header_len] = bytes(updated_info)
    return fw_data, updated_info


def main():
    
    parser = argparse.ArgumentParser(description="Patch firmware binary header with length, version, and CRC32.")
    parser.add_argument("-i", "--input", required=True, help="Input firmware binary (.bin)")
    parser.add_argument("-o", "--output", help="Output firmware binary (defaults to overwriting input in-place)")
    parser.add_argument("-v", "--version", type=lambda x: int(x, 0), default=0x00010000, help="Firmware version (e.g., 0x00010000)")
    parser.add_argument("-d", "--device-id", type=lambda x: int(x, 0), default=DEVICE_ID, help="Target Device ID (e.g., 0x14)")

    args = parser.parse_args()
    output_path = args.output if args.output else args.input

    try:
        with open(args.input, "rb") as f:
            raw_file = bytearray(f.read())

        patched_file, info = patch_firmware(raw_file, device_id=args.device_id, version=args.version)

        with open(output_path, "wb") as f:
            f.write(patched_file)

        print(f"[PATCH] {output_path}: Length=0x{info.length:X} ({info.length}B), CRC32=0x{info.crc32:08X}, Ver=0x{info.version:08X}")

    except FirmwarePatchError as e:
        print(f"[PATCH ERROR] {e}", file=sys.stderr)
        sys.exit(1)

    with open("../example_transfer_firmware.new.bin", "wb") as f:
        f.write(patched_file)


if __name__ == "__main__":
    main()
import ctypes
import crcmod
import struct
import argparse
import sys

from cryptography.hazmat.primitives.asymmetric import mldsa


FW_INFO_SENTINEL = 0xC0FFEE00
DEVICE_ID = 0x14
VECTOR_TABLE_SIZE = 0x1AC  # sizeof(vector_table_t)

FW_INFO_OFFSET = VECTOR_TABLE_SIZE

MLDSA44_PUBLIC_KEY_BYTES = 1312
MLDSA44_SIGNATURE_BYTES = 2420


# ---------------------------------------------------------------------------
# PRIVATE KEY
# ---------------------------------------------------------------------------
#
# This is the 32-byte ML-DSA-44 private seed.
#
# KEEP THIS SECRET.
#
# Anyone possessing this value can generate valid firmware signatures.
#
PRIVATE_KEY_SEED = bytes([
    0x61, 0x7d, 0x99, 0xd8, 0xf6, 0x8b, 0x86, 0x09,
    0xfc, 0x81, 0x49, 0x86, 0xac, 0x48, 0x4d, 0xa8,
    0xa9, 0x8c, 0x51, 0xa2, 0x27, 0x56, 0x39, 0xeb,
    0x2e, 0x52, 0x57, 0x1d, 0xfe, 0x6a, 0xbe, 0xa4,
])


# Reconstruct the ML-DSA-44 private key.
private_key = mldsa.MLDSA44PrivateKey.from_seed_bytes(
    PRIVATE_KEY_SEED
)


# ---------------------------------------------------------------------------
# STM32 CRC-32
# ---------------------------------------------------------------------------
#
# STM32 CRC-32 configuration:
# Poly:   0x104C11DB7
# Init:   0xFFFFFFFF
# Rev:    False
# XORout: 0x00000000
#
stm32_crc_func = crcmod.mkCrcFun(
    poly=0x104C11DB7,
    initCrc=0xFFFFFFFF,
    rev=False,
    xorOut=0x00000000
)


def stm32_crc32_fast(data: bytes) -> int:
    """
    Calculate the CRC in the same way the STM32 hardware CRC
    calculates it when reading 32-bit words from memory.
    """

    # Ensure 32-bit alignment.
    remainder = len(data) % 4

    if remainder != 0:
        data = data + b'\x00' * (4 - remainder)

    formatted_data = bytearray()

    for i in range(0, len(data), 4):
        # STM32 reads the word from little-endian memory.
        word = struct.unpack('<I', data[i:i + 4])[0]

        # CRC peripheral processes the word MSB-first.
        formatted_data.extend(struct.pack('>I', word))

    return stm32_crc_func(bytes(formatted_data))


# ---------------------------------------------------------------------------
# C STRUCT
# ---------------------------------------------------------------------------
#
# C:
#
# typedef struct {
#   uint32_t sentinel;
#   uint32_t device_id;
#   uint32_t version;
#   uint32_t length;
#   uint32_t _reserved0;
#   uint32_t _reserved1;
#   uint32_t _reserved2;
#   uint32_t _reserved3;
#   uint32_t _reserved4;
#   uint32_t crc32;
#   uint8_t signature[MLDSA44_BYTES];
# } firmware_info_t;
#
#
# 10 * uint32_t = 40 bytes
# signature       = 2420 bytes
# total           = 2460 bytes
#

class FirmwareInfo(ctypes.LittleEndianStructure):
    _pack_ = 1

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
        ("signature", ctypes.c_uint8 * MLDSA44_SIGNATURE_BYTES),
    ]

    def __init__(self, **kwargs):
        # Set default 0xFFFFFFFF padding for reserved fields.
        for field in (
            "_reserved0",
            "_reserved1",
            "_reserved2",
            "_reserved3",
            "_reserved4",
        ):
            kwargs.setdefault(field, 0xFFFFFFFF)

        kwargs.setdefault(
            "signature",
            (ctypes.c_uint8 * MLDSA44_SIGNATURE_BYTES)()
        )

        super().__init__(**kwargs)


# ---------------------------------------------------------------------------
# ERRORS
# ---------------------------------------------------------------------------

class FirmwarePatchError(Exception):
    """Base exception for all firmware patching errors."""


class FirmwareSizeError(FirmwarePatchError):
    """Firmware is too small to contain the required header."""


class InvalidSentinelError(FirmwarePatchError):
    """Firmware sentinel doesn't match."""


class InvalidDeviceIdError(FirmwarePatchError):
    """Firmware device ID doesn't match."""


class InvalidKeyError(FirmwarePatchError):
    """Private key is invalid."""


# ---------------------------------------------------------------------------
# FIRMWARE PATCHING
# ---------------------------------------------------------------------------

def patch_firmware(
    fw_data: bytearray,
    device_id: int,
    version: int
):
    """
    Patch firmware header, calculate CRC, and generate ML-DSA-44 signature.

    The CRC and signature are both calculated over the firmware payload
    immediately following fw_info_t:

        [vector table]
        [fw_info_t]
        [payload]  <-- CRC + signature

    The signature is then stored inside fw_info_t.signature.
    """

    header_len = ctypes.sizeof(FirmwareInfo)

    payload_offset = FW_INFO_OFFSET + header_len

    # ---------------------------------------------------------------
    # Verify sizes
    # ---------------------------------------------------------------

    if len(fw_data) < payload_offset:
        raise FirmwareSizeError(
            f"Firmware size ({len(fw_data)} bytes) is smaller than "
            f"the required header offset ({payload_offset} bytes)."
        )

    # ---------------------------------------------------------------
    # Verify ctypes structure matches the C structure
    # ---------------------------------------------------------------

    expected_header_len = 40 + MLDSA44_SIGNATURE_BYTES

    if header_len != expected_header_len:
        raise FirmwarePatchError(
            f"FirmwareInfo size mismatch: ctypes={header_len}, "
            f"expected={expected_header_len}"
        )

    # ---------------------------------------------------------------
    # Read existing firmware info
    # ---------------------------------------------------------------

    existing_header_bytes = fw_data[
        FW_INFO_OFFSET:
        FW_INFO_OFFSET + header_len
    ]

    existing_info = FirmwareInfo.from_buffer_copy(
        existing_header_bytes
    )

    # ---------------------------------------------------------------
    # Validate sentinel
    # ---------------------------------------------------------------

    if existing_info.sentinel != FW_INFO_SENTINEL:
        raise InvalidSentinelError(
            f"Invalid sentinel: "
            f"expected 0x{FW_INFO_SENTINEL:08X}, "
            f"got 0x{existing_info.sentinel:08X}."
        )

    # ---------------------------------------------------------------
    # Validate device ID
    # ---------------------------------------------------------------

    if existing_info.device_id != device_id:
        raise InvalidDeviceIdError(
            f"Invalid device ID: "
            f"expected 0x{device_id:02X}, "
            f"got 0x{existing_info.device_id:02X}."
        )

    # ---------------------------------------------------------------
    # Extract payload
    # ---------------------------------------------------------------

    payload = bytes(fw_data[payload_offset:])

    # ---------------------------------------------------------------
    # Calculate CRC
    # ---------------------------------------------------------------

    payload_crc = stm32_crc32_fast(payload)

    # ---------------------------------------------------------------
    # Calculate ML-DSA signature
    # ---------------------------------------------------------------
    #
    # IMPORTANT:
    #
    # The signature covers exactly the same bytes as the CRC.
    #
    # The fw_info structure itself is NOT signed.
    #
    signature = private_key.sign(payload)

    if len(signature) != MLDSA44_SIGNATURE_BYTES:
        raise FirmwarePatchError(
            f"Unexpected ML-DSA-44 signature size: "
            f"{len(signature)} bytes"
        )

    # ---------------------------------------------------------------
    # Construct updated firmware info
    # ---------------------------------------------------------------

    signature_array = (
        ctypes.c_uint8 * MLDSA44_SIGNATURE_BYTES
    ).from_buffer_copy(signature)

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
        signature=signature_array,
    )

    # ---------------------------------------------------------------
    # Write updated header back into firmware
    # ---------------------------------------------------------------

    fw_data[
        FW_INFO_OFFSET:
        FW_INFO_OFFSET + header_len
    ] = bytes(updated_info)

    return fw_data, updated_info


# ---------------------------------------------------------------------------
# MAIN
# ---------------------------------------------------------------------------

def main():

    parser = argparse.ArgumentParser(
        description=(
            "Patch firmware binary with length, version, CRC32, "
            "and ML-DSA-44 signature."
        )
    )

    parser.add_argument(
        "-i",
        "--input",
        required=True,
        help="Input firmware binary (.bin)"
    )

    parser.add_argument(
        "-o",
        "--output",
        help="Output firmware binary "
             "(defaults to overwriting input in-place)"
    )

    parser.add_argument(
        "-v",
        "--version",
        type=lambda x: int(x, 0),
        default=0x00010000,
        help="Firmware version (e.g. 0x00010000)"
    )

    parser.add_argument(
        "-d",
        "--device-id",
        type=lambda x: int(x, 0),
        default=DEVICE_ID,
        help="Target Device ID (e.g. 0x14)"
    )

    args = parser.parse_args()

    output_path = (
        args.output
        if args.output
        else args.input
    )

    try:

        with open(args.input, "rb") as f:
            raw_file = bytearray(f.read())

        patched_file, info = patch_firmware(
            raw_file,
            device_id=args.device_id,
            version=args.version
        )

        with open(output_path, "wb") as f:
            f.write(patched_file)

        print(
            f"[PATCH] {output_path}: "
            f"Length=0x{info.length:X} ({info.length}B), "
            f"CRC32=0x{info.crc32:08X}, "
            f"Ver=0x{info.version:08X}, "
            f"Signature={len(info.signature)}B"
        )

    except FirmwarePatchError as e:
        print(
            f"[PATCH ERROR] {e}",
            file=sys.stderr
        )
        sys.exit(1)


if __name__ == "__main__":
    main()

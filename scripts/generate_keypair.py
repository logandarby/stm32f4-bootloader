from cryptography.hazmat.primitives.asymmetric import mldsa


def format_python_bytes(name: str, data: bytes) -> str:
    """Format bytes as a Python bytes literal."""
    lines = []

    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        hex_values = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {hex_values},")

    return (
        f"{name} = bytes([\n"
        + "\n".join(lines)
        + "\n])"
    )


def format_c_array(name: str, data: bytes) -> str:
    """Format bytes as a C uint8_t array."""
    lines = []

    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        hex_values = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"    {hex_values},")

    return (
        f"static const uint8_t {name}[{len(data)}] = {{\n"
        + "\n".join(lines)
        + "\n};"
    )


def main():
    # Generate a new ML-DSA-44 keypair.
    private_key = mldsa.MLDSA44PrivateKey.generate()
    public_key = private_key.public_key()

    # cryptography uses the 32-byte seed as the raw private-key representation.
    private_seed = private_key.private_bytes_raw()

    # Raw ML-DSA-44 public key: 1312 bytes.
    public_key_bytes = public_key.public_bytes_raw()

    assert len(private_seed) == 32
    assert len(public_key_bytes) == 1312

    print()
    print("=== PRIVATE KEY / SEED ===")
    print()
    print(format_python_bytes("PRIVATE_KEY_SEED", private_seed))

    print()
    print("=== PUBLIC KEY ===")
    print()
    print("#include <stdint.h>")
    print()
    print(format_c_array("MLDSA44_PUBLIC_KEY", public_key_bytes))

    print()
    print("=== SIZES ===")
    print(f"Private seed: {len(private_seed)} bytes")
    print(f"Public key:   {len(public_key_bytes)} bytes")


if __name__ == "__main__":
    main()
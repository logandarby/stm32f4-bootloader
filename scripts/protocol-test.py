#!/usr/bin/env python3
import argparse
import logging
import sys
import time
from dataclasses import dataclass, field
from typing import Optional, Tuple

import serial
from rich.console import Console
from rich.logging import RichHandler
from rich.panel import Panel
from rich.table import Table

# Protocol Constants
PACKET_DATA_LEN = 16
PACKET_TOTAL_BYTES = 18

PACKET_BYTE_ACK = 0x15
PACKET_BYTE_RETX = 0x19
PACKET_BYTE_PADDING = 0xFF

console = Console()


def compute_crc8(data: bytes, poly: int = 0x07, init: int = 0x00) -> int:
    """Computes CRC-8 over input bytes matching standard CRC-8 implementation."""
    crc = init
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ poly) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


@dataclass
class Packet:
    length: int
    data: bytes = field(default_factory=lambda: bytes([PACKET_BYTE_PADDING] * PACKET_DATA_LEN))
    crc: int = 0
    
    def __str__(self) -> str:
        """Custom string representation for clean printing and logging."""
        if self.is_ack():
            return f"Packet(Type=ACK, CRC=0x{self.crc:02X})"
        if self.is_retx():
            return f"Packet(Type=ReTx, CRC=0x{self.crc:02X})"
        
        payload = self.data[:self.length]
        hex_data = payload.hex(" ")
        ascii_data = "".join(chr(b) if 32 <= b <= 126 else "." for b in payload)
        return (
            f"Packet(Length={self.length}, "
            f"Data=[{hex_data}], "
            f"ASCII='{ascii_data}', "
            f"CRC=0x{self.crc:02X})"
        )

    def __post_init__(self):
        # Ensure data is padded or truncated to 16 bytes
        if len(self.data) < PACKET_DATA_LEN:
            self.data = self.data + bytes([PACKET_BYTE_PADDING] * (PACKET_DATA_LEN - len(self.data)))
        elif len(self.data) > PACKET_DATA_LEN:
            self.data = self.data[:PACKET_DATA_LEN]

    def compute_crc(self) -> int:
        header_and_data = bytes([self.length]) + self.data
        return compute_crc8(header_and_data)

    def update_crc(self) -> None:
        self.crc = self.compute_crc()

    def serialize(self) -> bytes:
        return bytes([self.length]) + self.data + bytes([self.crc])

    @classmethod
    def deserialize(cls, raw_bytes: bytes) -> "Packet":
        if len(raw_bytes) != PACKET_TOTAL_BYTES:
            raise ValueError(f"Packet must be exactly {PACKET_TOTAL_BYTES} bytes")
        length = raw_bytes[0]
        data = raw_bytes[1:17]
        crc = raw_bytes[17]
        return cls(length=length, data=data, crc=crc)

    def is_ack(self) -> bool:
        return (
            self.length == 1
            and self.data[0] == PACKET_BYTE_ACK
            and all(b == PACKET_BYTE_PADDING for b in self.data[1:])
        )

    def is_retx(self) -> bool:
        return (
            self.length == 1
            and self.data[0] == PACKET_BYTE_RETX
            and all(b == PACKET_BYTE_PADDING for b in self.data[1:])
        )

    @classmethod
    def make_ack(cls) -> "Packet":
        data = bytes([PACKET_BYTE_ACK]) + bytes([PACKET_BYTE_PADDING] * (PACKET_DATA_LEN - 1))
        pkt = cls(length=1, data=data)
        pkt.update_crc()
        return pkt

    @classmethod
    def make_retx(cls) -> "Packet":
        data = bytes([PACKET_BYTE_RETX]) + bytes([PACKET_BYTE_PADDING] * (PACKET_DATA_LEN - 1))
        pkt = cls(length=1, data=data)
        pkt.update_crc()
        return pkt


class PacketProtocolHost:
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 0.1):
        self.logger = logging.getLogger("PacketProtocol")
        self.ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        self.last_transmitted: Optional[Packet] = None

        # State machine tracking
        self.state = "LENGTH"
        self.rx_buffer = bytearray()
        self.temp_length = 0
        self.temp_data = bytearray()

    def send_packet(self, packet: Packet) -> None:
        packet.update_crc()
        self.last_transmitted = packet
        raw = packet.serialize()
        self.ser.write(raw)
        self.logger.debug(f"[TX] {self._describe_packet(packet)} | Raw: {raw.hex(' ')}")

    def _describe_packet(self, pkt: Packet) -> str:
        if pkt.is_ack():
            return "SPECIAL: ACK"
        if pkt.is_retx():
            return "SPECIAL: RETX"
        payload = pkt.data[: pkt.length]
        ascii_repr = "".join(chr(b) if 32 <= b <= 126 else "." for b in payload)
        return f"Data (len={pkt.length}): [{payload.hex(' ')}] ('{ascii_repr}') CRC=0x{pkt.crc:02X}"

    def process_incoming_byte(self, byte_val: int) -> Optional[Packet]:
        """Byte-by-byte state machine matching embedded decoder."""
        if self.state == "LENGTH":
            self.temp_length = byte_val
            self.temp_data = bytearray()
            self.state = "DATA"

        elif self.state == "DATA":
            self.temp_data.append(byte_val)
            if len(self.temp_data) == PACKET_DATA_LEN:
                self.state = "CRC"

        elif self.state == "CRC":
            received_crc = byte_val
            pkt = Packet(length=self.temp_length, data=bytes(self.temp_data), crc=received_crc)
            expected_crc = pkt.compute_crc()

            self.state = "LENGTH"  # Reset state machine for next frame

            if received_crc != expected_crc:
                self.logger.error(
                    f"[RX BAD CRC] Received=0x{received_crc:02X}, Expected=0x{expected_crc:02X}. Sending ReTx.\n\tFull data recieved: {pkt}"
                )
                self.send_packet(Packet.make_retx())
                return None

            if pkt.is_retx():
                self.logger.warning("[RX] ReTx Request received from Target. Resending last packet.")
                if self.last_transmitted:
                    self.send_packet(self.last_transmitted)
                else:
                    self.logger.error("ReTx requested but no packet has been sent yet!")
                return None

            if pkt.is_ack():
                self.logger.info("[RX] ACK received from Target.")
                return None

            # Valid data packet received
            self.logger.info(f"[RX DATA] {self._describe_packet(pkt)}")
            self.send_packet(Packet.make_ack())
            return pkt

        return None

    def listen_forever(self) -> None:
        """Main event loop: waits for incoming packets and logs them."""
        self.logger.info(f"Listening on {self.ser.port} @ {self.ser.baudrate} baud...")
        try:
            while True:
                data = self.ser.read(64)
                if data:
                    for b in data:
                        pkt = self.process_incoming_byte(b)
                time.sleep(0.001)
        except KeyboardInterrupt:
            self.logger.info("Host listener stopped by user.")


def setup_logging(level_name: str) -> None:
    numeric_level = getattr(logging, level_name.upper(), logging.DEBUG)
    logging.basicConfig(
        level=numeric_level,
        format="%(message)s",
        datefmt="[%X]",
        handlers=[RichHandler(console=console, rich_tracebacks=True, markup=True)],
    )


def main():
    parser = argparse.ArgumentParser(description="UART Embedded Packet Protocol Host Test Utility")
    parser.add_argument("port", type=str, help="Serial port (e.g., /dev/ttyUSB0, /dev/ttyACM0, COM3)")
    parser.add_argument("-b", "--baudrate", type=int, default=115200, help="Baudrate (default: 115200)")
    parser.add_argument(
        "-l",
        "--log-level",
        type=str,
        default="DEBUG",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"],
        help="Logging verbosity (default: DEBUG)",
    )

    args = parser.parse_args()
    setup_logging(args.log_level)

    try:
        host = PacketProtocolHost(port=args.port, baudrate=args.baudrate)
        host.listen_forever()
    except serial.SerialException as e:
        console.print(f"[bold red]Serial Error:[/bold red] {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
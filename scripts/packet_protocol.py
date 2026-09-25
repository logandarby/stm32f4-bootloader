#!/usr/bin/env python3
import argparse
import logging
import sys
import time
from dataclasses import dataclass, field
from typing import Optional

import serial
from rich.console import Console
from rich.logging import RichHandler

# Protocol Constants
PACKET_SOF_BYTE = 0xAA
PACKET_DATA_LEN = 16
PACKET_TOTAL_BYTES = 19
PACKET_MAX_RETX_ATTEMPTS = 3

PACKET_CTRL_NONE = 0x00
PACKET_CTRL_ACK = 0x10
PACKET_CTRL_RETX = 0x20
PACKET_CTRL_MASK = 0xF0
PACKET_LEN_MASK = 0x0F

PACKET_BYTE_PADDING = 0xFF

console = Console()


def compute_crc8(data: bytes, poly: int = 0x07, init: int = 0x00) -> int:
    """Computes CRC-8 matching embedded implementation."""
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
    length: int  # Actual payload length (1 to 16)
    control: int = PACKET_CTRL_NONE
    data: bytes = field(default_factory=lambda: bytes([PACKET_BYTE_PADDING] * PACKET_DATA_LEN))
    crc: int = 0

    def __post_init__(self):
        if not (1 <= self.length <= 16):
            raise ValueError(f"Payload length must be between 1 and 16, got {self.length}")

        if len(self.data) < PACKET_DATA_LEN:
            self.data = self.data + bytes([PACKET_BYTE_PADDING] * (PACKET_DATA_LEN - len(self.data)))
        elif len(self.data) > PACKET_DATA_LEN:
            self.data = self.data[:PACKET_DATA_LEN]

    @property
    def header(self) -> int:
        encoded_len = (self.length - 1) & PACKET_LEN_MASK
        return (self.control & PACKET_CTRL_MASK) | encoded_len

    def __str__(self) -> str:
        if self.is_ack():
            return f"Packet(Type=ACK, CRC=0x{self.crc:02X})"
        if self.is_retx():
            return f"Packet(Type=ReTx, CRC=0x{self.crc:02X})"

        payload = self.data[: self.length]
        hex_data = payload.hex(" ")
        ascii_data = "".join(chr(b) if 32 <= b <= 126 else "." for b in payload)
        return (
            f"Packet(Length={self.length}, "
            f"Data=[{hex_data}], "
            f"ASCII='{ascii_data}', "
            f"CRC=0x{self.crc:02X})"
        )

    def compute_crc(self) -> int:
        header_and_data = bytes([self.header]) + self.data
        return compute_crc8(header_and_data)

    def update_crc(self) -> None:
        self.crc = self.compute_crc()

    def serialize(self) -> bytes:
        return bytes([PACKET_SOF_BYTE, self.header]) + self.data + bytes([self.crc])

    def is_ack(self) -> bool:
        return (self.control & PACKET_CTRL_MASK) == PACKET_CTRL_ACK

    def is_retx(self) -> bool:
        return (self.control & PACKET_CTRL_MASK) == PACKET_CTRL_RETX

    @classmethod
    def make_ack(cls) -> "Packet":
        pkt = cls(length=1, control=PACKET_CTRL_ACK)
        pkt.update_crc()
        return pkt

    @classmethod
    def make_retx(cls) -> "Packet":
        pkt = cls(length=1, control=PACKET_CTRL_RETX)
        pkt.update_crc()
        return pkt


class PacketProtocolHost:
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 0.1):
        self.logger = logging.getLogger("PacketProtocol")
        self.ser = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        self.last_transmitted: Optional[Packet] = None
        self.retx_counter = 0

        # State machine parameters
        self.state = "SOF"
        self.temp_header = 0
        self.temp_data = bytearray()

    def send_packet(self, packet: Packet) -> None:
        packet.update_crc()
        if not packet.is_ack() and not packet.is_retx():
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
        if self.state == "SOF":
            if byte_val == PACKET_SOF_BYTE:
                self.state = "HEADER"

        elif self.state == "HEADER":
            self.temp_header = byte_val
            self.temp_data = bytearray()
            self.state = "DATA"

        elif self.state == "DATA":
            self.temp_data.append(byte_val)
            if len(self.temp_data) == PACKET_DATA_LEN:
                self.state = "CRC"

        elif self.state == "CRC":
            received_crc = byte_val
            control = self.temp_header & PACKET_CTRL_MASK
            length = (self.temp_header & PACKET_LEN_MASK) + 1

            pkt = Packet(length=length, control=control, data=bytes(self.temp_data), crc=received_crc)
            expected_crc = pkt.compute_crc()

            self.state = "SOF"  # Reset state machine for next packet

            if received_crc != expected_crc:
                self.retx_counter += 1
                self.logger.error(
                    f"[RX BAD CRC] Received=0x{received_crc:02X}, Expected=0x{expected_crc:02X} (Attempt {self.retx_counter}/{PACKET_MAX_RETX_ATTEMPTS})."
                )
                if self.retx_counter <= PACKET_MAX_RETX_ATTEMPTS:
                    self.send_packet(Packet.make_retx())
                else:
                    self.logger.critical("Maximum ReTx attempts reached! Aborting packet transaction.")
                    self.retx_counter = 0
                return None

            if pkt.is_retx():
                self.retx_counter += 1
                self.logger.warning(
                    f"[RX] ReTx Request received from Target (Attempt {self.retx_counter}/{PACKET_MAX_RETX_ATTEMPTS})."
                )
                if self.retx_counter <= PACKET_MAX_RETX_ATTEMPTS:
                    if self.last_transmitted:
                        self.send_packet(self.last_transmitted)
                    else:
                        self.logger.error("ReTx requested but no packet cached!")
                else:
                    self.logger.critical("Maximum ReTx limit reached! Stopping retransmissions.")
                    self.retx_counter = 0
                return None

            # Valid packet received -> Reset counter
            self.retx_counter = 0

            if pkt.is_ack():
                self.logger.debug("[RX] ACK received from Target.")
                return None

            self.logger.debug(f"[RX DATA] {self._describe_packet(pkt)}")
            self.send_packet(Packet.make_ack())
            return pkt

        return None

    def listen_forever(self) -> None:
        self.logger.info(f"Listening on {self.ser.port} @ {self.ser.baudrate} baud...")
        try:
            while True:
                data = self.ser.read(64)
                if data:
                    for b in data:
                        self.process_incoming_byte(b)
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
    parser.add_argument("port", type=str, help="Serial port (e.g., /dev/ttyUSB0, COM3)")
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
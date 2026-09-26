#!/usr/bin/env python3
import argparse
import logging
import sys
import time
from collections import deque
from pathlib import Path
from typing import Optional

import serial
import serial.tools.list_ports
from packet_protocol import (
    PACKET_CTRL_ACK,
    PACKET_CTRL_MASK,
    PACKET_CTRL_NONE,
    PACKET_DATA_LEN,
    Packet,
    PacketProtocolHost,
    console,
    setup_logging,
)
from patch_firmware import (
    patch_firmware,
    FirmwareSizeError,
    InvalidSentinelError,
    InvalidDeviceIdError,
)
from rich.progress import BarColumn, Progress, SpinnerColumn, TextColumn, TimeRemainingColumn

# Protocol Constants matching firmware-transfer.h
FW_DEFAULT_TIMEOUT_S = 3.0
FW_ERASE_TIMEOUT_S = 15.0
DEFAULT_DEVICE_ID = 0x14
FIRMWARE_VERSION_NUMER = 0x00010000

# Sentinel Bytes
FW_BYTE_SEQ_OBSERVED = 0xA1
FW_BYTE_UPDATE_REQ = 0xA2
FW_BYTE_UPDATE_RES = 0xA3
FW_BYTE_DEVICE_ID_REQ = 0xA4
FW_BYTE_DEVICE_ID_RES = 0xA5
FW_BYTE_FW_LEN_REQ = 0xA6
FW_BYTE_FW_LEN_RES = 0xA7
FW_BYTE_READY = 0xA8
FW_BYTE_FW_UPDATE_SUCCESSFUL = 0xA9
FW_BYTE_NACK = 0xAB

# Sync Sequence Bytes
SYNC_SEQUENCE = bytes([0xC1, 0xC3, 0xC5, 0xC7])


class FirmwareTransferHost(PacketProtocolHost):
    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 0.05):
        super().__init__(port, baudrate=baudrate, timeout=timeout)
        self.rx_queue: deque[Packet] = deque()
        self.ack_received: bool = False

    def close(self) -> None:
        """Closes the underlying serial port connection."""
        if hasattr(self, "ser") and self.ser and self.ser.is_open:
            self.ser.close()

    def process_incoming_byte(self, byte_val: int) -> Optional[Packet]:
        """Intercepts protocol byte decoding to capture low-level ACKs while queuing data packets."""
        is_ack = self.state == "CRC" and (self.temp_header & PACKET_CTRL_MASK) == PACKET_CTRL_ACK
        pkt = super().process_incoming_byte(byte_val)

        if is_ack and self.state == "SOF":
            self.ack_received = True

        return pkt

    def poll_serial(self) -> None:
        """Reads all available bytes from serial and queues parsed packets."""
        if self.ser.in_waiting:
            data = self.ser.read(self.ser.in_waiting)
            for b in data:
                pkt = self.process_incoming_byte(b)
                if pkt is not None:
                    self.rx_queue.append(pkt)

    def wait_for_packet(self, timeout: float = FW_DEFAULT_TIMEOUT_S) -> Packet:
        """Blocks until a valid data packet is received or times out."""
        start_time = time.time()
        while time.time() - start_time < timeout:
            if self.rx_queue:
                pkt = self.rx_queue.popleft()
                if self._is_single_byte(pkt, FW_BYTE_NACK):
                    raise RuntimeError("Target rejected operation and issued NACK!")
                return pkt
            self.poll_serial()
            time.sleep(0.001)
        raise TimeoutError(f"Timed out waiting for target response after {timeout}s.")

    def send_and_wait_ack(self, packet: Packet, timeout: float = 1.0, max_retries: int = 3) -> None:
        """Sends a packet and waits for lower-level ACK before returning."""
        for attempt in range(1, max_retries + 1):
            self.ack_received = False
            self.send_packet(packet)
            start_time = time.time()
            while time.time() - start_time < timeout:
                self.poll_serial()
                if self.ack_received:
                    return
                time.sleep(0.001)
            self.logger.warning(
                f"[TX TIMEOUT] ACK not received for packet (Attempt {attempt}/{max_retries})"
            )
        raise TimeoutError("Failed to receive packet ACK from target.")

    def send_raw_sync(self) -> None:
        """Sends raw byte sync sequence without packet encapsulation."""
        self.ser.write(SYNC_SEQUENCE)
        self.logger.debug(f"[TX RAW SYNC] {SYNC_SEQUENCE.hex(' ')}")

    @staticmethod
    def _is_single_byte(pkt: Packet, expected_byte: int) -> bool:
        return pkt.length == 1 and pkt.data[0] == expected_byte

    def perform_update(
        self,
        firmware_bytes: bytes,
        device_id: int = DEFAULT_DEVICE_ID,
        skip_sync: bool = False,
    ) -> None:
        
        self.logger.info(f"Injecting Firmware with Firmware Info")
        firmware_bytes, _ = patch_firmware(bytearray(firmware_bytes), device_id=DEFAULT_DEVICE_ID, version=0x00010000)

        
        fw_len = len(firmware_bytes)
        self.logger.info(f"Beginning Firmware Transfer Protocol (Size: {fw_len} bytes)")

        # Step 1: Sync Phase
        if not skip_sync:
            self.logger.info("Attempting synchronization with bootloader...")
            synced = False
            sync_attempts = 0
            max_sync_attempts = 50

            while not synced and sync_attempts < max_sync_attempts:
                sync_attempts += 1
                self.send_raw_sync()
                try:
                    pkt = self.wait_for_packet(timeout=0.1)
                    if self._is_single_byte(pkt, FW_BYTE_SEQ_OBSERVED):
                        synced = True
                        self.logger.info("Target sync acknowledged (FW_BYTE_SEQ_OBSERVED).")
                except TimeoutError:
                    time.sleep(0.05)

            if not synced:
                raise RuntimeError("Failed to synchronize with target bootloader.")
        else:
            self.logger.info("Target already synchronized during device scan.")

        # Step 2: Update Request
        self.logger.info("Sending update request...")
        req_pkt = Packet(length=1, control=PACKET_CTRL_NONE, data=bytes([FW_BYTE_UPDATE_REQ]))
        self.send_and_wait_ack(req_pkt)

        res_pkt = self.wait_for_packet(timeout=FW_DEFAULT_TIMEOUT_S)
        if not self._is_single_byte(res_pkt, FW_BYTE_UPDATE_RES):
            raise RuntimeError(f"Unexpected update response from target: {res_pkt}")
        self.logger.info("Target accepted update request.")

        # Step 3: Device ID Handshake
        dev_req_pkt = self.wait_for_packet(timeout=FW_DEFAULT_TIMEOUT_S)
        if not self._is_single_byte(dev_req_pkt, FW_BYTE_DEVICE_ID_REQ):
            raise RuntimeError(f"Expected FW_BYTE_DEVICE_ID_REQ, got: {dev_req_pkt}")

        self.logger.info(f"Target requested Device ID. Sending 0x{device_id:02X}...")
        id_res_pkt = Packet(
            length=2,
            control=PACKET_CTRL_NONE,
            data=bytes([FW_BYTE_DEVICE_ID_RES, device_id & 0xFF]),
        )
        self.send_and_wait_ack(id_res_pkt)

        # Step 4: Firmware Length Handshake
        len_req_pkt = self.wait_for_packet(timeout=FW_DEFAULT_TIMEOUT_S)
        if not self._is_single_byte(len_req_pkt, FW_BYTE_FW_LEN_REQ):
            raise RuntimeError(f"Expected FW_BYTE_FW_LEN_REQ, got: {len_req_pkt}")

        self.logger.info(f"Sending firmware length ({fw_len} bytes)...")
        len_bytes = fw_len.to_bytes(4, byteorder="little")
        len_res_pkt = Packet(
            length=5,
            control=PACKET_CTRL_NONE,
            data=bytes([FW_BYTE_FW_LEN_RES]) + len_bytes,
        )
        self.send_and_wait_ack(len_res_pkt)

        # Step 5: Flash Erase Wait
        self.logger.info("Target erasing flash memory (this may take a few seconds)...")
        erase_ready_pkt = self.wait_for_packet(timeout=FW_ERASE_TIMEOUT_S)
        if not self._is_single_byte(erase_ready_pkt, FW_BYTE_READY):
            raise RuntimeError(f"Expected FW_BYTE_READY post-erase, got: {erase_ready_pkt}")
        self.logger.info("Flash erased. Starting payload transfer...")

        # Step 6: Firmware Data Transfer
        chunks = [firmware_bytes[i : i + PACKET_DATA_LEN] for i in range(0, fw_len, PACKET_DATA_LEN)]

        with Progress(
            SpinnerColumn(),
            TextColumn("[progress.description]{task.description}"),
            BarColumn(),
            TextColumn("[progress.percentage]{task.percentage:>3.0f}%"),
            TimeRemainingColumn(),
            console=console,
        ) as progress:
            task = progress.add_task("[green]Flashing firmware...", total=fw_len)

            total_chunks = len(chunks)
            for idx, chunk in enumerate(chunks):
                is_last_chunk = idx == total_chunks - 1
                chunk_pkt = Packet(length=len(chunk), control=PACKET_CTRL_NONE, data=chunk)
                self.send_and_wait_ack(chunk_pkt)
                res_pkt = self.wait_for_packet(timeout=FW_DEFAULT_TIMEOUT_S)
                if not is_last_chunk:
                    if not self._is_single_byte(res_pkt, FW_BYTE_READY):
                        raise RuntimeError(f"Expected FW_BYTE_READY, got: {res_pkt}")
                else:
                    # Final chunk must return SUCCESSFUL
                    if not self._is_single_byte(res_pkt, FW_BYTE_FW_UPDATE_SUCCESSFUL):
                        raise RuntimeError(
                            f"Expected FW_BYTE_FW_UPDATE_SUCCESSFUL, got: {res_pkt}"
                        )

                progress.advance(task, len(chunk))

        self.logger.info("Payload transfer complete and target verified successfully.")
        console.print("[bold green]Firmware update successfully completed![/bold green]")


def probe_port(port_name: str, baudrate: int = 115200, sync_attempts: int = 3) -> Optional[FirmwareTransferHost]:
    """
    Attempts to send SYNC_SEQUENCE on a given port.
    If target responds, returns the open, synchronized FirmwareTransferHost instance.
    Otherwise closes the connection and returns None.
    """
    try:
        host = FirmwareTransferHost(port=port_name, baudrate=baudrate, timeout=0.05)
        for _ in range(sync_attempts):
            host.send_raw_sync()
            try:
                pkt = host.wait_for_packet(timeout=0.1)
                if host._is_single_byte(pkt, FW_BYTE_SEQ_OBSERVED):
                    return host
            except TimeoutError:
                time.sleep(0.02)
        host.close()
    except (serial.SerialException, OSError, PermissionError):
        pass
    return None


def scan_for_target_ports(baudrate: int = 115200) -> list[tuple[str, FirmwareTransferHost]]:
    """Scans all available COM/TTY ports and returns active host instances for matching targets."""
    available_ports = serial.tools.list_ports.comports()
    if not available_ports:
        return []

    matching_targets = []
    for port_info in available_ports:
        dev = port_info.device
        console.print(f"Checking port [cyan]{dev}[/cyan] ({port_info.description})...", end=" ")
        host = probe_port(dev, baudrate)
        if host:
            console.print("[bold green]TARGET FOUND[/bold green]")
            matching_targets.append((dev, host))
        else:
            console.print("[dim]No response[/dim]")
    return matching_targets


def main():
    parser = argparse.ArgumentParser(description="Target Firmware Flasher Host Utility")
    parser.add_argument("firmware", type=Path, help="Path to binary firmware image (.bin)")
    parser.add_argument(
        "-p",
        "--port",
        type=str,
        default=None,
        help="Serial port (e.g., /dev/ttyUSB0, COM3). Omit to scan all connected USB ports.",
    )
    parser.add_argument("-b", "--baudrate", type=int, default=115200, help="Baudrate (default: 115200)")
    parser.add_argument(
        "-d",
        "--device-id",
        type=lambda x: int(x, 0),
        default=DEFAULT_DEVICE_ID,
        help=f"Target Device ID (default: 0x{DEFAULT_DEVICE_ID:02X})",
    )
    parser.add_argument(
        "-l",
        "--log-level",
        type=str,
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR"],
        help="Logging verbosity (default: INFO)",
    )

    args = parser.parse_args()
    setup_logging(args.log_level)

    if not args.firmware.is_file():
        console.print(f"[bold red]Error:[/bold red] Firmware file '{args.firmware}' not found.")
        sys.exit(1)

    firmware_bytes = args.firmware.read_bytes()
    if len(firmware_bytes) == 0:
        console.print("[bold red]Error:[/bold red] Firmware image is empty.")
        sys.exit(1)

    target_host: Optional[FirmwareTransferHost] = None
    already_synced = False

    if args.port:
        # User passed a specific port, instantiate host and require standard sync phase
        target_host = FirmwareTransferHost(port=args.port, baudrate=args.baudrate)
        already_synced = False
    else:
        # Auto-detect mode
        console.print("[bold yellow]No serial port specified. Scanning available USB/serial ports...[/bold yellow]")
        found_targets = scan_for_target_ports(args.baudrate)

        if not found_targets:
            console.print("[bold red]Error:[/bold red] No responsive bootloader target found on any USB port.")
            sys.exit(1)

        # Select the first discovered device and mark it as already synced
        selected_port, target_host = found_targets[0]
        already_synced = True

        # Close any additional matching devices if multiple were found
        for _, extra_host in found_targets[1:]:
            extra_host.close()

        if len(found_targets) == 1:
            console.print(f"[bold green]Target detected on port:[/bold green] {selected_port}\n")
        else:
            all_ports_str = ", ".join(dev for dev, _ in found_targets)
            console.print(
                f"[bold yellow]Multiple targets found ({all_ports_str}).[/bold yellow] "
                f"Selecting first port: {selected_port}\n"
            )

    try:
        target_host.perform_update(
            firmware_bytes=firmware_bytes,
            device_id=args.device_id,
            skip_sync=already_synced,
        )
    except (serial.SerialException, RuntimeError, TimeoutError) as e:
        console.print(f"\n[bold red]Transfer Failed:[/bold red] {e}")
        sys.exit(1)
    finally:
        target_host.close()


if __name__ == "__main__":
    main()
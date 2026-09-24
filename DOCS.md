Here is your updated Markdown document with the **UART** section filled out to accurately document your peripheral setup:

# Bootloader Documentation

## Preliminary Information


* C Standard: C99
* Size: 32KiB
* Start Addr: `0x0800 0000`
* End Addr: `0x0800 7FFF`

The bootloader lives in the first two sectors of flash memory. The firmware lives in sectors (2 - 7) from addresses `0x0800 8000` - `0x0807 FFFF`.

## Firmware Update Mechanism

The following protocol layers are used. Custom protocols are elaborated in [Custom Protocols](https://www.google.com/search?q=%2523custom-protocols&utm_source=gemini)

* Layer 0: UART
* Layer 1: [Packet Transfer](https://www.google.com/search?q=%2523packet-transfer&utm_source=gemini)
* Layer 2: [Firmware Transfer](https://www.google.com/search?q=%2523firmware-transfer&utm_source=gemini)

## Protocol Specifications & Definitions

### UART

The physical layer uses standard 8N1 asynchronous serial communication operating on STM32 **USART2**.

| Parameter | Configuration |
| --- | --- |
| Baud Rate | 115200 bps |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Total Bits | 10 |
| Flow Control | None |
| Pin Mapping | TX: PA2, RX: PA3 (Alternate Function AF7) |
| RX Mechanism | Interrupt-driven (`USART2_IRQ`) into a 128-byte software ring buffer |
| TX Mechanism | Polled / Blocking (`usart_send_blocking`) |

### Packet Transfer

Packets are

* 18 bytes
* Byte 0: Length OR Special Packet Sentinel
* Bytes 1-16: Data
* Byte 17: CRC8

Special Packets/Bytes:

| Byte | Value | Description |
| --- | --- | --- |
| 1 | `0x15` | ACK - Acknowledge |
| 1 | `0x19` | ReTx - Request Retransmit |
| Data Bytes | `0xFF` | Padding - Must be used when data is not meant to occupy data slots |

The following state machine diagram represents the protocol used to transfer packets of data over UART

```mermaid
flowchart TD
    A[Receive<br/>length byte] --> B[Receive data<br/>bytes]
    B --> C[Receive CRC<br/>byte]

    C -->|Is ReTx| D[Retransmit<br/>last packet]
    D --> A

    C -->|Bad CRC| E[Send ReTx]
    E --> A
    C -->|Is ACK| A
    C -->|Is Data| F[Transmit<br/>acknowledge]
    F --> G[Store packet<br/>in buffer]
    G --> A


```

### Firmware Transfer

The following state machine diagram represents the protocol for the user and hardware initiating a firmware transfer and update.

```mermaid
flowchart LR
    START([Wait for sync])

    START --> SYNC[Send synced message]
    SYNC --> SYNC_OK{Sync acknowledged?}

    SYNC_OK -->|Yes| UPDATE_REQ[Wait for firmware update request]
    SYNC_OK -->|No / Timeout| SYNC_FAIL[Sync failed]
    SYNC_FAIL --> MAIN([Jump to main application])

    UPDATE_REQ --> UPDATE_MSG{Update request received?}
    UPDATE_MSG -->|Yes| DEVICE_REQ[Request firmware device ID]
    UPDATE_MSG -->|No / Timeout| UPDATE_FAIL[Update request timeout]
    UPDATE_FAIL --> MAIN

    DEVICE_REQ --> DEVICE_ID{Device ID matches?}
    DEVICE_ID -->|Yes| LENGTH_REQ[Request firmware length]
    DEVICE_ID -->|No / Timeout| DEVICE_FAIL[Device ID mismatch]
    DEVICE_FAIL --> MAIN

    LENGTH_REQ --> LENGTH_RX{Firmware length received?}
    LENGTH_RX -->|Yes| LENGTH_VALID{Firmware size valid?}
    LENGTH_RX -->|No / Timeout| LENGTH_FAIL[Failed to receive firmware length]
    LENGTH_FAIL --> MAIN

    LENGTH_VALID -->|Yes| RECEIVE[Receive firmware]
    LENGTH_VALID -->|No / Timeout| SIZE_FAIL[Firmware too large]
    SIZE_FAIL --> MAIN

    RECEIVE --> FW_VALID{Firmware transfer valid?}
    FW_VALID -->|Yes| COMPLETE[Firmware update complete]
    FW_VALID -->|No / Timeout| FW_FAIL[Firmware transfer failed]
    FW_FAIL --> MAIN

    COMPLETE --> MAIN

```
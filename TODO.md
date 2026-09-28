## top priority

- some kind of signature on firmware
- rollback. use sector 5 and 6 for firmware A and B, each 128KiB. Sector 0 and 1 can be the bootloader, and sector 3 can contain bootloader metadata for
  - anti-downgrade protection
  - metadata for rollback. something like magic, state (confirmed, pending, testing), active slot, pending slot, attempts, version, crc
  - Have to solve absolute address problem. Maybe B sector is a staging area

## other

Add protection against downgrade attack using versioning and checking firmware struct
explore hashing maybe
use ECDSA for signature
stop embedding bootloader into firmware? Ship with default firmware and create new dir for firmware we can transfer



from chat jipidi:

> hello. i've built an educational bootloader for the stm32f401re, implementing these things:
> - drivers for gpio, uart, crc, etc.
> - firmware transfer over uart.
> - firmware transfer over 3 protocols-- uart, a custom packet protocol, then a firmware protocol
> - firmware integrity check using crc peripheral
> - bootloader protection with MPU
> - Basic anti-rollback protection (just versioning in the firmware, nothing stored non-volatile)
> 
> this is for educational purposes. the only other thing i want to implement is maybe a staged firmware system (two sections to hold firmware, i can overwrite one at a time, non-volatile register flip to determine) and a cryptographic signature (educational of course, i don't want to use OTP or hold a signing key somewhere).
> 
> this is for a resume project. What are some other good features to add?

For a resume project, I’d prioritize features that demonstrate **embedded-systems design depth**, rather than adding lots of protocol features.

 - **A/B firmware + atomic boot selection** — excellent choice. Add boot-attempt counters, rollback after failed boots, and power-loss-safe state transitions.
- **Cryptographic verification** — implement ECDSA/Ed25519 or RSA signature verification, with the public key compiled into the bootloader. You don’t need to store a private signing key on the MCU.
- **Image metadata/header format** — magic, version, image size, CRC/hash, signature, hardware compatibility, build ID, entry point, etc.
- **SHA-256 hashing** — complements your CRC nicely: CRC for accidental corruption, cryptographic hash/signature for authenticity.
- **Fault-tolerant update state machine** — explicitly handle resets/power loss at every stage.
- **Watchdog integration** — bootloader/application handshake and watchdog-based recovery.
- **Vector-table/application validation** — verify stack pointer, reset handler address, flash boundaries, and image metadata before jumping.
- **Memory protection refinement** — separate bootloader/application regions and configure MPU permissions deliberately.
- **Boot diagnostics** — persistent boot reason, failed-update reason, reset cause, firmware version, and update status.
- **Automated host-side flashing tool** — Python CLI that packages, signs, transfers, verifies, and reports progress. This can make the project feel much more complete.
- **Test/fault-injection framework** — deliberately interrupt transfers, corrupt packets, reset during updates, and verify recovery.

 For a resume, I’d especially emphasize **A/B updates + power-loss recovery + signed images + host-side tooling + automated fault testing**. That demonstrates considerably more engineering than simply adding another communication protocol.
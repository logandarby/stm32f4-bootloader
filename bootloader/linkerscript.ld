/* Enforce emmition of the vector table. */
EXTERN(vector_table)
/* Define the entry point of the output file. */
ENTRY(reset_handler)
/* Enforce emission of slot regions for default firmware */
EXTERN(default_fw_slot_a)
EXTERN(bootloader_metadata_fw)

/* Define memory regions. */
MEMORY
{
    rom (rx)    : ORIGIN = 0x08000000, LENGTH = 32K /* Bootloader */
		meta (r)	  : ORIGIN = 0x08008000, LENGTH = 16K /* Bootloader metadata */
		slot_a (rx) : ORIGIN = 0x08020000, LENGTH = 128K   /* Slot A (Sector 5) */
    slot_b (rx) : ORIGIN = 0x08040000, LENGTH = 128K   /* Slot B (Sector 6) */
    slot_scratch (rx) : ORIGIN = 0x08060000, LENGTH = 128K   /* Scratch (Sector 7) */
		ram (rwx)   : ORIGIN = 0x20000000, LENGTH = 96K
}

/* Define sections. */
SECTIONS
{
	.text : {
		*(.vectors)	/* Vector table */
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
	} >rom

	.preinit_array : {
			. = ALIGN(4);
			__preinit_array_start = .;
			KEEP (*(.preinit_array))
			__preinit_array_end = .;
	} >rom

	.init_array : {
			. = ALIGN(4);
			__init_array_start = .;
			KEEP (*(SORT(.init_array.*)))
			KEEP (*(.init_array))
			__init_array_end = .;
	} >rom

	.fini_array : {
			. = ALIGN(4);
			__fini_array_start = .;
			KEEP (*(.fini_array))
			KEEP (*(SORT(.fini_array.*)))
			__fini_array_end = .;
	} >rom

	.ARM.extab : {
			*(.ARM.extab*)
	} >rom

	.ARM.exidx : {
			__exidx_start = .;
			*(.ARM.exidx*)
			__exidx_end = .;
	} >rom

	. = ALIGN(4);
	_etext = .;

	.bootloader_metadata_fw : {
			. = ALIGN(4);
			_boot_metadata_start = .;
			KEEP(*(.bootloader_metadata))
			. = ALIGN(4);
	} >meta

	/* Embed Default Firmware Payload into Slot A & Pad */
	.slot_a_fw : {
			_slot_a_start = .;
			KEEP(*(.slot_a_data))
	} >slot_a

	.slot_b_fw : {
			_slot_b_start = .;
			KEEP(*(.slot_b_data))
	} >slot_b

	.slot_scratch_fw : {
			_slot_scratch_start = .;
	} >slot_scratch

	/* ram, but not cleared on reset, eg boot/app comms */
	.noinit (NOLOAD) : {
		*(.noinit*)
	} >ram
	. = ALIGN(4);

	.data : {
		_data = .;
		*(.data*)	/* Read-write initialized data */
		*(.ramtext*)	/* "text" functions to run in ram */
		. = ALIGN(4);
		_edata = .;
	} >ram AT >rom
	_data_loadaddr = LOADADDR(.data);

	.bss : {
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >ram


	. = ALIGN(4);
	end = .;
}

PROVIDE(_stack = ORIGIN(ram) + LENGTH(ram));

/* Export Bootloader Metadata Sector symbols */
PROVIDE(_boot_metadata_start = ORIGIN(meta));
PROVIDE(_boot_metadata_end   = ORIGIN(meta) + LENGTH(meta));

_slot_a_max_size = LENGTH(slot_a);
_slot_b_max_size = LENGTH(slot_b);
_slot_scratch_max_size = LENGTH(slot_scratch);

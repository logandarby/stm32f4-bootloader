/* Enforce emmition of the vector table. */
EXTERN(vector_table)

/* Define the entry point of the output file. */
ENTRY(reset_handler)

/* Define memory regions. */
MEMORY
{
    rom (rx)  : ORIGIN = 0x0800C000, LENGTH = 208K
		ram (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

/* Define sections. */
SECTIONS
{
	.text : {
		_firmware_start = .;

		_vector_table_start = .;
		*(.vectors)	/* Vector table */
		_vector_table_end = .;

		. = ALIGN(4);

		_firmware_info_start = .;
		KEEP (*(.firmware_info))	/* Firmware specific info to store on device during transfer  */
		_firmware_info_end = .;

		. = ALIGN(4);
		
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

/* Export explicit size constants for usage in C code / patch scripts */
PROVIDE(_vector_table_size = _vector_table_end - _vector_table_start);
PROVIDE(_firmware_info_size = _firmware_info_end - _firmware_info_start);
PROVIDE(_payload_size = _payload_end - _payload_start);

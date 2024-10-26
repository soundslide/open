ENTRY(_vectors)

SECTIONS
{
   .text ROM_START : {
	   *(.stack_ptr*)
	   *(.interrupts*)

		# This pointer must be in the same position for all firmware versions.
		# That's why it is placed right after interrupts.
	   moveAndReset = . + UPLOAD_START;
	   *(.mover_ptr)

	   *(.text*)
   }

   # we run new mover installed in uploader buffer starting at UPLOAD_START
   # therefore VMA needs to be set to higher half of RAM
   .mover ADDR(.text) + SIZEOF(.text) + UPLOAD_START : AT ( ADDR(.text) + SIZEOF(.text) ) {
	   *(.mover*)
   }

	# now we need to rewind the counter back to lower half of RAM
   .rodata ADDR(.mover) + SIZEOF(.mover) - UPLOAD_START : AT ( ADDR(.mover) + SIZEOF(.mover) - UPLOAD_START ) {
		. = ALIGN(4);
		*(.rodata*)
		. = ALIGN(4);

		_init_array_start = .;
		. = ALIGN(4);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		. = ALIGN(4);
		_init_array_end = .;

		_data_src = . ;
   }

   .data RAM_START : AT ( _data_src ) {
		_data_start = . ;
		*(.data*)
		. = ALIGN(4);
		_data_end = . ;

		_stack_top = RAM_SIZE ;
   }

   .bss _data_end : {
		. = ALIGN(4);
	   _bss_start = . ;
	   *(.bss*)
	   *(COMMON*)
		. = ALIGN(4);
	   _bss_end = . ;
   }

}

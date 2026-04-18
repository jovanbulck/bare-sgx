OUTPUT_FORMAT(elf64-x86-64)

PHDRS
{
    text    PT_LOAD FLAGS(5);   /* r-x */
    data    PT_LOAD FLAGS(6);   /* rw- */
    stack   PT_LOAD FLAGS(6);   /* rw- */
}

SECTIONS
{
	. = 0;
	__enclave_base = .;

	/* unmapped guard page */
	. += 0x1000;

	.text : ALIGN(4096) {
		*(.text*)
		*(.rodata*)
		FILL(0xcc);
	} : text

	.data : ALIGN(4096) {
		*(.data*)
	} : data

	/* unmapped guard page */
 	. = ALIGN(4096);
	. += 0x1000;

	.stack : ALIGN(4096) {
		*(.stack*)
	} : stack

	/DISCARD/ : {
		*(.comment*)
		*(.note*)
		*(.debug*)
		*(.eh_frame*)
		*(.dyn*)
		*(.gnu.hash)
	}
}

ASSERT(!DEFINED(_GLOBAL_OFFSET_TABLE_), "Libcalls through GOT are not supported in enclaves")

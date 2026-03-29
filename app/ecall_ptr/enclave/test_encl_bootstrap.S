	.text
encl_entry:
        lea     encl_stack(%rip), %rax
	xchg	%rsp, %rax
	push	%rax

	push	%rcx # push the address after EENTER

        # restore calling convention
        mov     %rsi, %rdi
        mov     %rdx, %rsi

	# NOTE: as the selftest enclave is *not* intended for production,
	# simplify the code by not initializing ABI registers on entry or
	# cleansing caller-save registers on exit.
	call	encl_body

	# Prepare EEXIT target by popping the address of the instruction after
	# EENTER to RBX.
	pop	%rbx

	# Restore the caller stack.
	pop	%rax
	mov	%rax, %rsp

	# EEXIT
	mov	$4, %rax
	enclu

	.section .data
	.balign 4096
	# Stack of TCS #1
	.space 4096
encl_stack:

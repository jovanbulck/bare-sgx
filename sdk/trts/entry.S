#include "baresgx/config.h"
	.text
	/*
	 * EENTER:
	 *	- rax: CSSA   -- 0 = ecall ; >= 1 exception handler
	 *	- rbx: &TCS   -- address of the current TCS
	 *	- rcx: &eexit -- address of instruction following EENTER
	 */
encl_entry:
	cmp $1, %rax
    je .exception_entry

.regular_entry:
    lea     encl_stack(%rip), %rax
	xchg	%rsp, %rax

	# save u_rsp and eexit_target
	push	%rax
	push	%rcx

	# Sanitize CPU state: x86-64 ABI requires RFLAGS.DF=0 on function
	# entry, and we additionally clear RFLAGS.AC to prevent #AC-fault side
	# channels.
	# NOTE: Real-world enclave runtimes should also cleanse extended CPU
	# state (i.e., x87 FPU and SSE/AVX/...) configuration registers,
	# preferably using XRSTOR. This is currently _not_ supported for bare-sgx
	# enclaves, which should not use any floating-point instructions or
	# extended CPU state.
	cld
	pushfq
	andq 	$~0x40000, (%rsp)
	popfq

	# call dispatch function with attacker argument
    mov     %rsi, %rdi
	call	encl_body

	# Clear general purpose registers (excl XRSTOR for now, cf above)
	xor		%rax, %rax
	xor     %rcx, %rcx
	xor     %rdx, %rdx
	xor     %rbx, %rbx
	xor     %rbp, %rbp
	xor     %rsi, %rsi
	xor     %rdi, %rdi
	xor     %r8,  %r8
	xor     %r9,  %r9
	xor     %r10, %r10
	xor     %r11, %r11
	xor     %r12, %r12
	xor     %r13, %r13
	xor     %r14, %r14
	xor     %r15, %r15
	add     %rax, %rax # OF = SF = AF = CF = 0; ZF = PF = 1

#if BARE_TRTS_ECALL_CLEANSE_STACK
	# Opt-in defense-in-depth: wipe .stack except the top 2 quadwords
	lea     encl_stack_top(%rip), %rdi
	mov     $(BARE_TRTS_NB_STACK_PAGES*4096)/8-2, %rcx
	rep stosq
	xor     %rdi, %rdi
	xor     %rcx, %rcx
#endif

.restore_caller_stack:
	# Restore eexit_target and ursp
	pop	    %rbx
	pop	    %rsp

	# EEXIT
	mov		$4, %rax
	enclu
	ud2

.exception_entry:
#if BARE_TRTS_EXCEPTION_HANDLING
	# Set up enclave stack
	lea     encl_exc_stack(%rip), %r11
	xchg	%rsp, %r11
	push	%r11

	# Push the return address (provided in RCX by EENTER)
	push	%rcx

	# pass CSSA (in RAX) as 3rd argument (RDX) to the C handler
	# and TCS (in RBX) as 4th argument (RCX)
    mov     %rsi, %rdi
    mov     %rdx, %rsi
	mov		%rax, %rdx
	mov		%rbx, %rcx

	# call the C-level exception handler inside the enclave
	call	encl_exception_handler

	jmp .restore_caller_stack
#else
	ud2
#endif

    .global get_enclave_base
get_enclave_base:
	endbr64
	lea 	__enclave_base(%rip), %rax
	ret

	.global get_enclave_size
get_enclave_size:
	endbr64
	mov 	__enclave_size(%rip), %rax
	ret

	.section .stack, "aw", @nobits
	.balign 4096
encl_stack_top:
	.space BARE_TRTS_NB_STACK_PAGES*4096
encl_stack:

#if BARE_TRTS_EXCEPTION_HANDLING
	.balign 4096
	.space 4096
encl_exc_stack:
#endif

	# The following 8 bytes (measured as part of MRENCLAVE) will be
	# filled in by the untrusted loader with the total size of the
	# loaded enclave.
    .section .rodata, "a", @progbits
    .global __enclave_size
__enclave_size:
    .quad 0x0

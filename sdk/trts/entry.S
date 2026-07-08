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
	mov     %rbx, %rsi
	call	encl_body

.cleanup:
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

	# save u_rsp and eexit_target
	push	%r11
	push	%rcx

	# save CSSA and TCS
	push    %rax
    push    %rbx

	# pass CSSA (in RAX) as 2rd argument (RDX) to the C handler
	# and TCS (in RBX) as 3th argument (RCX)
    mov     %rsi, %rdi
    mov     %rax, %rsi
	mov		%rbx, %rdx

	# call the C-level exception handler inside the enclave
	call	encl_exception_handler


	# Restore TCS and CSSA
	pop     %rbx
    pop     %rax

#if BARE_TRTS_AEX_NOTIFY

	# Restore eexit_target and ursp
	pop	    %rbx
	pop	    %rsp

.restore_ssa_0_state:
	/*
	* Calculate SSA[0] frame adress
	* frame = TCS + 4096 + (CSSA-1)*4096 = TCS + CSSA*4096
	* sgx_gpr is last 184 bytes of ssa frame: 4096 - 184 = 3912
	*/
	mov 	%rbx, %rcx		# RCX = TCS
	add     $4096, %rcx		# RCX = SSA[0]
	add     $3912, %rcx		# RCX = sgx_gpr

	/* restore other registers */
    mov     8*5(%rcx), %rbp
    mov     8*6(%rcx), %rsi
    mov     8*7(%rcx), %rdi
    mov     8*8(%rcx), %r8
    mov     8*9(%rcx), %r9
    mov     8*10(%rcx), %r10
    mov     8*11(%rcx), %r11
    mov     8*12(%rcx), %r12
    mov     8*13(%rcx), %r13
    mov     8*14(%rcx), %r14
    mov     8*15(%rcx), %r15
    # NOTE: we do this on SSA[1] stack _before_ restoring SSA[0] stack to avoid
    # clobbering the red zone or smashex vulnerabilities(!)
    # following instructions cannot modify flags
    push    8*16(%rcx) 			# rflags
    popf
    mov     8*4(%rcx), %rsp

    .global atomic_start
atomic_start:
	# EDECCSSA
    movq	$9, %rax
    enclu

	lea	encl_stack(%rip), %rdx 	# stack base addres

	movq aex_rip(%rip), %rbx	# interrputed instruction
	movl    $12, %ecx
	shrx    %rcx, %rbx, %rbx
	shlx    %rcx, %rbx, %rbx	# Align rbx to a 4KB page boundary

	# Prefetch all cache lines in stack page and code page
	mov $4096, %ecx
.prefetch_loop:
	lea -0x40(%ecx), %ecx 		# ecx -= 64
	mov (%rdx, %rcx), %rax		# stack page tickle
	mov (%rbx, %rcx), %rax		# code page tickle
	jrcxz   .restore_last_regs  # if ecx == 0, exit loop
    jmp     .prefetch_loop 		# loop back (loops 64 times)



.restore_last_regs:
    # Restore RAX and jump to RIP
    movq   	aex_rax(%rip), %rax
    movq   	aex_rcx(%rip), %rcx
	movq   	aex_rdx(%rip), %rdx
	movq   	aex_rbx(%rip), %rbx
	jmp		*aex_rip(%rip)

    .global atomic_end
atomic_end:

#else
	jmp .cleanup
#endif

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

#if BARE_TRTS_AEX_NOTIFY

	/*
	 * Manual wrappers to ensure these are PIC-relative to enclave base
	 */
	.global get_atomic_start
get_atomic_start:
	lea atomic_start(%rip), %rax
	ret

	.global get_atomic_end
get_atomic_end:
	lea atomic_end(%rip), %rax
	ret

.data
# Global vars to hold register values during resumption flow.
	.balign 8
    .global aex_rip
aex_rip:
    .quad 0
    .global aex_rax
aex_rax:
    .quad 0
    .global aex_rcx
aex_rcx:
    .quad 0
    .global aex_rdx
aex_rdx:
    .quad 0
    .global aex_rbx
aex_rbx:
    .quad 0
#endif

	# The following 8 bytes (measured as part of MRENCLAVE) will be
	# filled in by the untrusted loader with the total size of the
	# loaded enclave.
    .section .rodata, "a", @progbits
    .global __enclave_size
__enclave_size:
    .quad 0x0

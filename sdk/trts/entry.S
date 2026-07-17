#include "baresgx/config.h"
#include "internal/arch.h"
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

#if BARE_TRTS_EXCEPTION_HANDLING && BARE_TRTS_AEX_NOTIFY
    # Set frame->sgx_gpr.aex_notify = 1 for SSA[0]
    # sgx_gpr = TCS + 4096 + 3912
    mov	%rbx, %rcx
    add	$PAGE_SIZE+SSA_FRAME_BEFORE_GPR_SIZE, %rcx
    # aex_notify is the 167th byte within gprsgx
    movb	$1, 167(%rcx)
#endif

	# call dispatch function with attacker argument
    mov     %rsi, %rdi
	call	encl_body

.ecall_ret:
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
#if !BARE_TRTS_EXCEPTION_HANDLING
	ud2
#else
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
    mov     %rbx, %rdi

	# call the C-level exception handler inside the enclave
	call	encl_exception_handler

	# Restore TCS and CSSA
	pop     %rbx
    pop     %rax

#if !BARE_TRTS_AEX_NOTIFY
	jmp .ecall_ret
#else
	# discard eexit_target and ursp
	pop	    %rcx
	pop	    %rcx

	/*
	* Calculate SSA[0].sgx_gpr adress
	* frame = TCS + 4096 + (CSSA-1)*4096 = TCS + CSSA*4096
	* sgx_gpr is last 184 bytes of ssa frame: 4096 - 184 = 3912
	*
	* \note: current exception handling implementation assumes ssaframesize = 1 page
	*/
	mov 	%rbx, %rcx		                  			# RCX = TCS
  	add     $PAGE_SIZE+SSA_FRAME_BEFORE_GPR_SIZE, %rcx 	# RCX = &SSA[0].GPRSGX

    # Save interrupted state unless RIP is inside [atomic_start, atomic_end)
    mov     8*17(%rcx), %r9                             # r9 = RIP

    lea     atomic_start(%rip), %r10
    cmp     %r10, %r9
    jb      .restart_atomic_section

    lea     atomic_end(%rip), %r10
    cmp     %r10, %r9
    jb      .restore_ssa_0_state

.restart_atomic_section:
    mov     %r9, aex_rip(%rip)

    mov     8*0(%rcx), %r10
    mov     %r10, aex_rax(%rip)

    mov     8*1(%rcx), %r10
    mov     %r10, aex_rcx(%rip)

    mov     8*2(%rcx), %r10
    mov     %r10, aex_rdx(%rip)

    mov     8*3(%rcx), %r10
    mov     %r10, aex_rbx(%rip)

.restore_ssa_0_state:
	/* restore SSA[0] GPRs (except those used in the atomic stub)  */
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

    movq	$9, %rax
	.global atomic_start
atomic_start:
	# EDECCSSA : this will re-enable aexnotify and should be atomic
	# : this will #UD on procesors that do not support aexnotify
    enclu
	lea	encl_stack_top(%rip), %rdx 	# stack top addres

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
#endif

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

#if BARE_TRTS_EXCEPTION_HANDLING && BARE_TRTS_AEX_NOTIFY
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
#endif

	.section .stack, "aw", @nobits
	.balign 4096
encl_stack_top:
	.space BARE_TRTS_NB_STACK_PAGES*4096
encl_stack:

#if BARE_TRTS_EXCEPTION_HANDLING
	.balign 4096
	.space 4096
encl_exc_stack:

#if BARE_TRTS_AEX_NOTIFY
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
#endif

	# The following 8 bytes (measured as part of MRENCLAVE) will be
	# filled in by the untrusted loader with the total size of the
	# loaded enclave.
    .section .rodata, "a", @progbits
    .global __enclave_size
__enclave_size:
    .quad 0x0

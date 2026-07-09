/* SPDX-License-Identifier: GPL-2.0 */
/**
* Copyright(c) 2016-20 Intel Corporation.
*/

    .text
    .global baresgx_enter_enclave
baresgx_enter_enclave:
    # store callee-save registers
    push    %rbx
    push    %rbp
    push    %r12
    push    %r13
    push    %r14
    push    %r15

    # prepare EENTER
    mov     %rdi, %rbx
    mov     $2, %rax
    mov     baresgx_aep_pointer(%rip), %rcx
    enclu
    mov     %rsi, %rax

    # restore callee-save registers
    pop     %r15
    pop     %r14
    pop     %r13
    pop     %r12
    pop     %rbp
    pop     %rbx
    ret

    .global baresgx_default_aep
baresgx_default_aep:
    /*store rbx and rcx, then restore them after the exception handler*/
    push %rbx # RBX holding TCS
    push %rcx # RCX holding aep
    call c_aep_handler
    pop %rcx
    pop %rbx
    .global baresgx_aep_resume_assembly
baresgx_aep_resume_assembly:
    mov $3, %rax # ENCLU_ERESUME = 3
    enclu
    ud2 # should never be reached.

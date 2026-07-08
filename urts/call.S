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
    enclu

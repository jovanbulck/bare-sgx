/* SPDX-License-Identifier: GPL-2.0 */
/**
* Copyright(c) 2016-20 Intel Corporation.
*/

    .text
    .global baresgx_enter_enclave
baresgx_enter_enclave:
    mov $2, %rax
    mov %rdi, %rbx
    lea baresgx_aep(%rip), %rcx
    enclu

    mov %rsi, %rax
    ret

baresgx_aep:
    enclu

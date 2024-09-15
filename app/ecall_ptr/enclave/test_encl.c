// SPDX-License-Identifier: GPL-2.0
/*  Copyright(c) 2016-20 Intel Corporation. */

#include "test_encl.h"

static void do_encl_op_add(void *_op)
{
	struct encl_op_math *op = _op;

        /* TODO *insecure* ptr dereference: needs a check */
        *op->rv_pt = op->val1 + op->val2;
}

static void do_encl_op_sub(void *_op)
{
	struct encl_op_math *op = _op;

        /* TODO *insecure* ptr dereference: needs a check */
        *op->rv_pt = op->val1 - op->val2;
}

/*
 * Symbol placed at the start of the enclave image by the linker script.
 * Declare this extern symbol with visibility "hidden" to ensure the compiler
 * does not access it through the GOT and generates position-independent
 * addressing as __encl_base(%rip), so we can get the actual enclave base
 * during runtime.
 */
extern const uint8_t __attribute__((visibility("hidden"))) __encl_base;

typedef void (*encl_op_t)(void *);

/* NOTE: need to declare this volatile to preven the compiler from inlining the
 * accesses in encl_body and breaking the manual relocation.. */
volatile encl_op_t encl_op_array[ENCL_OP_MAX] = {
	do_encl_op_add,
	do_encl_op_sub,
};

void encl_body(void *rdi, void *rsi)
{
	struct encl_op_header *header = (struct encl_op_header *)rdi;
	encl_op_t op;

        /* TODO *insecure* ptr dereference: needs a check */
	if (header->type >= ENCL_OP_MAX)
		return;

	/*
	 * The enclave base address needs to be added, as this call site
	 * *cannot be* made rip-relative by the compiler, or fixed up by
	 * any other possible means.
	 */
	op = ((uint64_t)&__encl_base) + encl_op_array[header->type];
	(*op)(header);
}


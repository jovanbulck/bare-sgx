// SPDX-License-Identifier: GPL-2.0
/*  Copyright(c) 2016-20 Intel Corporation. */

#include "test_encl.h"
#include "baresgx/trts.h"

static void do_encl_op_add(void *u_op)
{
	struct encl_op_math op;
	SAFE_COPY_STRUCT(&op, u_op);
	ASSERT_OUTSIDE_ENCLAVE(op.rv_pt, sizeof(op.rv_pt));

    *op.rv_pt = op.val1 + op.val2;
}

static void do_encl_op_sub(void *u_op)
{
	struct encl_op_math op;
	SAFE_COPY_STRUCT(&op, u_op);
	ASSERT_OUTSIDE_ENCLAVE(op.rv_pt, sizeof(op.rv_pt));

    *op.rv_pt = op.val1 - op.val2;
}

static void do_encl_op_info(void *u_op)
{
	struct encl_op_info op;
	SAFE_COPY_STRUCT(&op, u_op);
	ASSERT_OUTSIDE_ENCLAVE(op.base_pt, sizeof(op.base_pt));
	ASSERT_OUTSIDE_ENCLAVE(op.size_pt, sizeof(op.size_pt));

    *op.base_pt = get_enclave_base();
    *op.size_pt = get_enclave_size();
}

/*
 * Symbol placed at the start of the enclave image by the linker script.
 * Declare this extern symbol with visibility "hidden" to ensure the compiler
 * does not access it through the GOT and generates position-independent
 * addressing as __enclave_base(%rip), so we can get the actual enclave base
 * during runtime.
 */
extern const uint8_t __attribute__((visibility("hidden"))) __enclave_base;

typedef void (*encl_op_t)(void *);

/* NOTE: need to declare this volatile to preven the compiler from inlining the
 * accesses in encl_body and breaking the manual relocation.. */
volatile encl_op_t encl_op_array[ENCL_OP_MAX] = {
	do_encl_op_add,
	do_encl_op_sub,
	do_encl_op_info,
};

void encl_body(void *rdi)
{
	encl_op_t op;
	struct encl_op_header header;

	SAFE_COPY_STRUCT(&header, rdi);
	if (header.type >= ENCL_OP_MAX)
		return;

	/*
	 * The enclave base address needs to be added, as this call site
	 * *cannot be* made rip-relative by the compiler, or fixed up by
	 * any other possible means.
	 */
	op = ((uint64_t)&__enclave_base) + encl_op_array[header.type];
	(*op)(rdi);
}


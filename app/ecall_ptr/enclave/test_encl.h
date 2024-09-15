#ifndef TEST_ENCL_H_INC
#define TEST_ENCL_H_INC

#include <stddef.h>
#include <stdint.h>

enum encl_op_type {
	ENCL_OP_ADD,
	ENCL_OP_SUB,
        ENCL_OP_MAX,
};

struct encl_op_header {
	uint64_t type;
};

struct encl_op_math {
	struct encl_op_header header;
        uint64_t val1;
        uint64_t val2;
        uint64_t *rv_pt;
};

#endif

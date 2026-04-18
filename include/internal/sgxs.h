#ifndef BARESGX_SGXS_H_INC
#define BARESGX_SGXS_H_INC

#include <stdint.h>
#include "sgx-defs.h"

#pragma pack(1)

/*
 * MRENCLAVE blocks as defined in Intel SDM.
 */

#define SGXS_TAG_ECREATE	0x0045544145524345 // "ECREATE\0"
#define SGXS_TAG_EADD		0x0000000044444145 // "EADD\0\0\0\0"
#define SGXS_TAG_EEXTEND   	0x00444E4554584545 // "EEXTEND\0"

#define EEXTEND_SIZE 		256

struct sgxs_ecreate {
	uint64_t tag;
	uint32_t ssaframesize;
	uint64_t size;
	uint8_t reserved[44];
};
_Static_assert(sizeof(struct sgxs_ecreate) == 64, "MRENCLAVE block size");

struct sgxs_eadd {
	uint64_t tag;
	uint64_t offset;
	uint64_t flags; /* SECINFO flags */
	uint8_t reserved[40];
};
_Static_assert(sizeof(struct sgxs_eadd) == 64, "MRENCLAVE block size");

struct sgxs_eextend {
	uint64_t tag;
	uint64_t offset;
	uint8_t reserved[48];
    /* Followed by 256 bytes of data */
	uint8_t  blob[EEXTEND_SIZE];
};
_Static_assert(sizeof(struct sgxs_eextend) == 5*64, "MRENCLAVE block size");

#endif

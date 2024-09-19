#ifndef BARESGX_ELF_ENCL_H_INC
#define BARESGX_ELF_ENCL_H_INC

/*
 * Definitions for Linux's custom selftest ELF enclave format.
 */

#include "sgx-defs.h"
#define PAGE_SIZE               4096
#define PAGE_MASK               (~(PAGE_SIZE - 1))

struct encl_segment {
	void *src;
	off_t offset;
	size_t size;
	unsigned int prot;
	unsigned int flags;
	bool measure;
};

struct encl {
	int fd;
	void *bin;
	off_t bin_size;
	void *src;
	size_t src_size;
	size_t encl_size;
	off_t encl_base;
	unsigned int nr_segments;
	struct encl_segment *segment_tbl;
	struct sgx_secs secs;
	struct sgx_sigstruct sigstruct;
};

/**
 * Map the binary enclave file in unprotected host memory and parse the ELF
 * headers to record the enclave segment layout in @param(struct encl).
 */
bool encl_load(const char *path, struct encl *encl, unsigned long heap_size);

/**
 * Compute the enclave signature and store in @param(encl->sigstruct).
 */
bool encl_measure(struct encl *encl);

/**
 * Create the enclave prepared in @param(encl->sigstruct) in EPC memory.
 */
bool encl_build(struct encl *encl, int debug);

void encl_delete(struct encl *encl);

void pretty_print_encl(const struct encl *enclave);
void print_encl_segment(const struct encl_segment *segment);

#endif

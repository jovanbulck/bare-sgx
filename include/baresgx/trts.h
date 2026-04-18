#ifndef BARE_TRTS_H_INC
#define BARE_TRTS_H_INC

#include <stddef.h>
#include <stdint.h>

#define ASSERT_OUTSIDE_ENCLAVE(u_pt, size)					 \
	do {													 \
		if (!u_pt || !sgx_is_outside_enclave(u_pt, size))	 \
			panic();										 \
	} while (0)

#define SAFE_COPY_STRUCT(t_pt, u_pt)					     \
    do { 												     \
		/* 1. check if the argument lies entirely outside */ \
		ASSERT_OUTSIDE_ENCLAVE(u_pt, sizeof(*t_pt));         \
		/* 2. copy the argument inside to prevent TOCTOU */	 \
		memcpy(t_pt,u_pt,sizeof(*t_pt));				     \
	} while(0)

#define sgx_lfence() asm("lfence");

void panic(void);

int memcmp(const void *s1, const void *s2, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);

int sgx_is_outside_enclave(void *addr, size_t len);
int sgx_is_within_enclave(void *addr, size_t len);

uint64_t get_enclave_base(void);
uint64_t get_enclave_size(void);

#endif
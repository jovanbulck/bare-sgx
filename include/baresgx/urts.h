#ifndef BARE_SGX_URTS_H_INC
#define BARE_SGX_URTS_H_INC
#include <stdint.h>
#include <asm/sgx.h>
#include "util.h"

/*
 * Load the enclave provided @param(path) in canonical SGXS enclave format.
 *
 * @return      load address of the first TCS in the enclave
 */
void* baresgx_load_sgxs_enclave(const char* sgxs_path, const char *sigstruct_path, int debug);

uint64_t baresgx_enter_enclave(void* tcs, uint64_t arg1);

/* Custom AEP get/set functions for SGX-Step */
void* sgx_get_aep(void);
void  sgx_set_aep(void* aep);
void* sgx_get_tcs(void);

#endif

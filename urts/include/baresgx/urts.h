#ifndef BARE_SGX_URTS_H_INC
#define BARE_SGX_URTS_H_INC

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <asm/sgx.h>
#include <stdlib.h>

#define BARESGX_DEBUG           0

#define BARESGX_ASSERT(cond)                                            \
    do {                                                                \
        if (!(cond))                                                    \
        {                                                               \
            perror("[" __FILE__ "] assertion '" #cond "' failed");      \
            abort();                                                    \
        }                                                               \
    } while(0)

#define baresgx_info(msg, ...)                                          \
    do {                                                                \
        printf("[" __FILE__ "] " msg "\n", ##__VA_ARGS__);              \
        fflush(stdout);                                                 \
    } while(0)

#if BARESGX_DEBUG
    #define baresgx_debug(msg, ...)                                     \
        do {                                                            \
            printf("[" __FILE__ "] " msg "\n", ##__VA_ARGS__);          \
            fflush(stdout);                                             \
        } while(0)
#else
    #define baresgx_debug(msg, ...)
#endif

/*
 * Load the enclave provided @param(path) in Linux's custom selftest ELF
 * enclave format.
 *
 * @return      load address of the first TCS in the enclave
 */
void* baresgx_load_elf_enclave(const char* path, int debug);

uint64_t baresgx_enter_enclave(void* tcs, uint64_t arg1);

/* Custom AEP get/set functions for SGX-Step */
void* sgx_get_aep(void);
void  sgx_set_aep(void* aep);
void* sgx_get_tcs(void);

#endif

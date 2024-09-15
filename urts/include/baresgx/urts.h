#ifndef BARE_SGX_URTS_H_INC
#define BARE_SGX_URTS_H_INC

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <asm/sgx.h>
#include <stdlib.h>

#define BARESGX_DEBUG           1

#define ASSERT(cond)                                                    \
    do {                                                                \
        if (!(cond))                                                    \
        {                                                               \
            perror("[" __FILE__ "] assertion '" #cond "' failed");      \
            abort();                                                    \
        }                                                               \
    } while(0)

#define info(msg, ...)                                                  \
    do {                                                                \
        printf("[" __FILE__ "] " msg "\n", ##__VA_ARGS__);              \
        fflush(stdout);                                                 \
    } while(0)

#if BARESGX_DEBUG
    #define debug(msg, ...)                                                 \
        do {                                                                \
            printf("[" __FILE__ "] " msg "\n", ##__VA_ARGS__);              \
            fflush(stdout);                                                 \
        } while(0)
#else
    #define debug(msg, ...)
#endif

/*
 * Load the enclave provided @param(path) in Linux's custom selftest ELF
 * enclave format.
 *
 * @return      load address of the first TCS in the enclave
 */
void* baresgx_load_elf_enclave(const char* path);

uint64_t baresgx_enter_enclave(void* tcs, uint64_t arg1);

#endif

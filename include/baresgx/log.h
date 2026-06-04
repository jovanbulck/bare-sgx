#ifndef LOG_H_INC
#define LOG_H_INC
#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "config.h"

/* ------------------------------------------------------------------ */
/* Debug/logging                                                      */
/* ------------------------------------------------------------------ */

#define BARESGX_ASSERT_RET(cond, param)                                  \
    if (!(cond))                                                         \
    {                                                                    \
        fprintf(stderr, "Error: bad parameter '%s'\n", param);           \
        return -1;                                                       \
    }                                                                    \

#define BARESGX_ASSERT(cond)                                                 \
    do {                                                                     \
        if (!(cond))                                                         \
        {                                                                    \
            if (errno != 0)                                                  \
                perror("["__FILE__"] assertion '"#cond"' failed");           \
            else                                                             \
                fprintf(stderr,"["__FILE__"] assertion '"#cond"' failed\n"); \
            exit(1);                                                         \
        }                                                                    \
    } while(0)

#define baresgx_error(msg, ...)                                         \
    do {                                                                \
        printf("[" __FILE__ "] error: " msg "\n", ##__VA_ARGS__);       \
        fflush(stdout);                                                 \
    } while(0)

#if BARE_URTS_LOG_LEVEL >= 1
    #define baresgx_info(msg, ...)                                          \
        do {                                                                \
            printf("[" __FILE__ "] " msg "\n", ##__VA_ARGS__);              \
            fflush(stdout);                                                 \
        } while(0)
#else
    #define baresgx_info(msg, ...)
#endif

#if BARE_URTS_LOG_LEVEL >= 2
    #define baresgx_debug(msg, ...)                                     \
        do {                                                            \
            printf("[" __FILE__ "] " msg "\n", ##__VA_ARGS__);          \
            fflush(stdout);                                             \
        } while(0)
#else
    #define baresgx_debug(msg, ...)
#endif

static inline char *hex_str(void *buf, size_t len)
{
    #define HEX_STR_SLOTS    8
    #define HEX_STR_MAX_LEN  256
    
    static char pool[HEX_STR_SLOTS][HEX_STR_MAX_LEN * 2 + 1];
    static unsigned int slot = 0;
    BARESGX_ASSERT(buf && len && len <= HEX_STR_MAX_LEN);

    char *out = pool[slot++ % HEX_STR_SLOTS];
    int off = 0, n;
    for (size_t i = 0; i < len; i++) {
        n = sprintf(out + off, "%02x", ((unsigned char*)buf)[i]);
        BARESGX_ASSERT(n == 2);
        off += n;
    }

    return out;
}

#endif

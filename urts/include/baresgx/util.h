#ifndef UTIL_H_INC
#define UTIL_H_INC
#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Debug/logging                                                      */
/* ------------------------------------------------------------------ */

#define BARESGX_DEBUG  0

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

#define baresgx_info(msg, ...)                                          \
    do {                                                                \
        printf("[" __FILE__ "] " msg "\n", ##__VA_ARGS__);              \
        fflush(stdout);                                                 \
    } while(0)

#define baresgx_error(msg, ...)                                         \
    do {                                                                \
        printf("[" __FILE__ "] error: " msg "\n", ##__VA_ARGS__);       \
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

/* ------------------------------------------------------------------ */
/* Bit flags                                                          */
/* ------------------------------------------------------------------ */

#define BIT(nr)					(1UL << (nr))

#define PAGE_SIZE               4096
#define PAGE_MASK               (~(PAGE_SIZE - 1))

#define X_MAKE_ENUM(name, bit) 	name = bit,
#define X_MAKE_MASK(name, bit)  bit |

#define X_MAKE_SPRINTF(name, bit)                                 \
    if ((flags) & (bit)) {                                        \
        n = snprintf(p+off, 512-off, "%s%s", !off?"":"|", #name); \
        BARESGX_ASSERT(n >= 0);                                   \
        off += n;                                                 \
	}

#define DEFINE_FLAGS_STR(name, enum_list_def)           \
    static char name##_str_buf[512];                    \
	static inline char* name##_to_str(uint64_t flags)   \
	{                                                   \
		char *p = name##_str_buf; 			            \
		int off = 0, n;                                 \
	    enum_list_def(X_MAKE_SPRINTF)                   \
	    if (!off)                                       \
	        sprintf(p, "NONE");                         \
	    return p;                                       \
	}

#define DEFINE_FLAGS_VALID(name, enum_list_def)			\
	static inline int name##_is_valid(uint64_t x) 		\
	{                                                   \
	    return !(x & ~(enum_list_def(X_MAKE_MASK) 0));  \
	}

#define MK_ENUM( name, enum_list_def)                   \
	enum name { 										\
		enum_list_def( X_MAKE_ENUM ) 					\
	};                                                  \
	DEFINE_FLAGS_STR(name, enum_list_def)               \
	DEFINE_FLAGS_VALID(name, enum_list_def)

#endif
#ifndef UTIL_H_INC
#define UTIL_H_INC
#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "../baresgx/log.h"
#include "arch.h"

/* ------------------------------------------------------------------ */
/* Bit flags                                                          */
/* ------------------------------------------------------------------ */

#define BIT(nr)					(1UL << (nr))

#define PAGE_MASK               (~(PAGE_SIZE - 1))
#define IS_PAGE_ALIGNED(p)      (((uintptr_t)(p) & (PAGE_SIZE - 1)) == 0)

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

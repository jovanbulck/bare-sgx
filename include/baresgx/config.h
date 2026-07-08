#ifndef CONFIG_H_INC
#define CONFIG_H_INC

/*
 * Log level for the untrusted runtime:
 *
 * \arg 0 = print only error msgs
 * \arg 1 = print info + error msgs
 * \arg 2 = print debug + info + error msgs
 */
#define BARE_URTS_LOG_LEVEL                     1

/*
 * Opt-in defense-in-depth feature: zeroes the enclave stack page between
 * ecalls to scrub residual secrets from previous calls.
 *
 * \note accessing uninitialized stack memory in the enclave is undefined
 * behavior; this zeroing provides no strict guarantees and should not be
 * relied upon as a primary mitigation.
 */
#define BARE_TRTS_ECALL_CLEANSE_STACK           0

/*
 * Number of 4 KiB pages to allocate for the enclave stack (default=1).
 *
 * \note bare-sgx automatically inserts an unmapped guard page under the stack
 * to protect against stack overflows.
 */
#define BARE_TRTS_NB_STACK_PAGES                1

/*
 * enclave exception handling support TODO expand explanation
 */
#define BARE_TRTS_EXCEPTION_HANDLING            1

/*
 * enclave aex notify support TODO expand explanation
 */
#define BARE_TRTS_AEX_NOTIFY                    1

#endif

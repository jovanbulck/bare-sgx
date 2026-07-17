#include <stdio.h>
#include "baresgx/urts.h"
#include "enclave/test_encl.h"

#include <signal.h>


#define ENCLAVE_SGXS    "enclave/encl.sgxs"
#define ENCLAVE_SIG     "enclave/encl.sig"
#define ENCLAVE_DEBUG   0
#define ENCLAVE_AEX_NOTIFY   0


extern uint64_t g_encl_base, g_encl_size;

void *tcs;


static void illegal_instruction_handler(int sig, siginfo_t *info, void *context)
{
    printf("\n--- Signal Caught ---\n");
    printf("Signal Number: %d (SIGILL)\n", sig);
    printf("Faulting Address: %p\n", info->si_addr);

    // Common code for SIGILL: ILL_ILLOPC (illegal opcode)
    if (info->si_code == ILL_ILLOPC)
    {
        printf("Reason: Illegal opcode.\n");
    }
    //printf("Exiting safely.\n");
    //exit(1);
    baresgx_enter_enclave(tcs, 0);
}

int main(void)
{
    struct sigaction sa_ill;

    sa_ill.sa_sigaction = illegal_instruction_handler;
    sigemptyset(&sa_ill.sa_mask);
    sa_ill.sa_flags = SA_SIGINFO;

    BARESGX_ASSERT(sigaction(SIGILL, &sa_ill, NULL) != -1);

    uint64_t encl_base = 0, encl_size = 0;
    struct encl_op_math arg;
    struct encl_op_info info = {
        .header = {.type = ENCL_OP_INFO},
        .base_pt = &encl_base,
        .size_pt = &encl_size,
    };
    uint64_t rv = -1;
    //void *tcs;

    tcs = baresgx_load_sgxs_enclave(ENCLAVE_SGXS, ENCLAVE_SIG, ENCLAVE_DEBUG, ENCLAVE_AEX_NOTIFY);
    baresgx_info("loaded enclave at %p", tcs);

    baresgx_info("reading enclave memory..");
    printf("\tL mem at %p is %lx\n", (void*) tcs, *((uint64_t*) tcs));

    baresgx_info("calling enclave TCS..");
    baresgx_enter_enclave(tcs, (uint64_t) &info);
    printf("\tL enclave returned base=%#lx; size=%#lx\n", encl_base, encl_size);
    BARESGX_ASSERT(g_encl_base == encl_base);
    BARESGX_ASSERT(g_encl_size == encl_size);

    arg.header.type = ENCL_OP_ADD;
    arg.val1 = 1300;
    arg.val2 = 37;
    arg.rv_pt = &rv;
    baresgx_enter_enclave(tcs, (uint64_t) &arg);
    printf("\tL enclave returned %ld + %ld = %ld\n", arg.val1, arg.val2, rv);

    arg.header.type = ENCL_OP_SUB;
    arg.val1 = 1300;
    arg.val2 = 37;
    arg.rv_pt = &rv;
    baresgx_enter_enclave(tcs, (uint64_t) &arg);
    printf("\tL enclave returned %ld - %ld = %ld\n", arg.val1, arg.val2, rv);

    struct encl_op_return return_op = {
        .header = {.type = ENCL_OP_RETURN},
        .rv_pt = &rv,
    };
    baresgx_enter_enclave(tcs, (uint64_t) &return_op);
    printf("\tL enclave returned CPUID result = %ld\n", rv);

    return 0;
}

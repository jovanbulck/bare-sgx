#define _GNU_SOURCE
#include <stdio.h>
#include "baresgx/urts.h"
#include "enclave/test_encl.h"

#include "libsgxstep/enclave.h"
#include "libsgxstep/debug.h"
#include "libsgxstep/cpu.h"

#include <signal.h>
#include <string.h>
#include <ucontext.h>


#define ENCLAVE_SGXS    "enclave/encl.sgxs"
#define ENCLAVE_SIG     "enclave/encl.sig"
#define ENCLAVE_DEBUG   1
#define ENCLAVE_AEX_NOTIFY   1

extern uint64_t g_encl_base, g_encl_size;

void *tcs;

/* Called before resuming the enclave after an Asynchronous Enclave eXit. */
void aep_cb_func(void)
{
    uint64_t erip = edbgrd_erip() - (uint64_t) get_enclave_base();
    uint32_t cssa = -1;
    edbgrd(sgx_get_tcs()+24, &cssa, 4);
    printf("^^ enclave RIP=%#lx; CSSA=%d\n", erip, cssa);

#if 0
    gprsgx_region_t gprsgx = {0};
    edbgrd(get_enclave_ssa_gprsgx_adrs(), &gprsgx, sizeof(gprsgx_region_t));
    dump_gprsgx_region(&gprsgx);
#endif

    if (cssa == 2)
        sgx_step_do_trap = 1;
    else
        sgx_step_do_trap = 0;
}

int step = 0;

void fault_handler(int signo, siginfo_t * si, void  *ctx)
{
    ucontext_t *uc = (ucontext_t *) ctx;

    switch ( signo )
    {
      case SIGTRAP:
        step++;

        /* ensure RFLAGS.TF is clear to disable debug single-stepping */
        uc->uc_mcontext.gregs[REG_EFL] &= ~0x100;
        break;

      default:
        info("Caught unknown signal '%d'", signo);
        abort();
    }

    // NOTE: return eventually continues at aep_cb_func and initiates
    // single-stepping mode.
}

void register_signal_handler(int signo)
{
    struct sigaction act, old_act;

    /* Specify #PF handler with signinfo arguments */
    memset(&act, 0, sizeof(sigaction));
    act.sa_sigaction = fault_handler;
    act.sa_flags = SA_RESTART | SA_SIGINFO;

    /* Block all signals while the signal is being handled */
    sigfillset(&act.sa_mask);
    ASSERT(!sigaction( signo, &act, &old_act ));
}

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

    //baresgx_enter_enclave(tcs, 0);
    //printf("returned from exception handling \n");
}

int main(void)
{
    struct sigaction sa_ill;

    sa_ill.sa_sigaction = illegal_instruction_handler;
    sigemptyset(&sa_ill.sa_mask);
    sa_ill.sa_flags = SA_SIGINFO;

    if (sigaction(SIGILL, &sa_ill, NULL) == -1)
    {
        perror("sigaction illegal_instruction_handler not regestered correctly");
        return 1;
    }

    uint64_t encl_base = 0, encl_size = 0;
    struct encl_op_math arg;
    struct encl_op_info info = {
        .header = {.type = ENCL_OP_INFO},
        .base_pt = &encl_base,
        .size_pt = &encl_size,
    };
    uint64_t rv = -1;

    tcs = baresgx_load_sgxs_enclave(ENCLAVE_SGXS, ENCLAVE_SIG, ENCLAVE_DEBUG, ENCLAVE_AEX_NOTIFY);
    baresgx_info("loaded enclave at %p", tcs);

    // HACK: bare-sgx enclaves start with an unmapped page which sgx-step
    // doesn't currently discover correctly
    extern struct sgx_step_enclave_info victim;
    register_enclave_info();
    victim.base -= 0x1000;

    print_enclave_info();
    dump_enclave_exec_pages();

    register_aep_cb(aep_cb_func);
    register_signal_handler( SIGTRAP );
    set_debug_optin();

    baresgx_info("reading enclave memory..");
    printf("\tL mem at %p is %lx\n", (void*) tcs, *((uint64_t*) tcs));

    baresgx_info("calling enclave TCS..");
    sgx_step_do_trap = 0;
    step = 0;
    baresgx_enter_enclave(tcs, (uint64_t) &info);
    printf("\tL enclave returned base=%#lx; size=%#lx; step=%d\n", encl_base, encl_size, step);
    BARESGX_ASSERT(g_encl_base == encl_base);
    BARESGX_ASSERT(g_encl_size == encl_size);
    dump_enclave_exec_pages();
    
#if 0
    arg.header.type = ENCL_OP_ADD;
    arg.val1 = 1300;
    arg.val2 = 37;
    arg.rv_pt = &rv;
    baresgx_enter_enclave(tcs, (uint64_t) &arg);
    printf("\tL enclave returned %ld + %ld = %ld\n", arg.val1, arg.val2, rv);
    baresgx_info("aep_counter after 2nd call = %d", get_aep_counter());

    reset_aep_counter();
    baresgx_info("aep_counter reset");
    arg.header.type = ENCL_OP_SUB;
    arg.val1 = 1300;
    arg.val2 = 37;
    arg.rv_pt = &rv;
    baresgx_enter_enclave(tcs, (uint64_t) &arg);
    printf("\tL enclave returned %ld - %ld = %ld\n", arg.val1, arg.val2, rv);
    baresgx_info("aep_counter after 3th call = %d", get_aep_counter());


    struct encl_op_return return_op = {
        .header = {.type = ENCL_OP_RETURN},
        .rv_pt = &rv,
    };
    baresgx_enter_enclave(tcs, (uint64_t) &return_op);
    printf("\tL enclave returned CPUID result = %ld\n", rv);
#endif

    printf("\n--- Test completed successfully ---\n");
    return 0;
}

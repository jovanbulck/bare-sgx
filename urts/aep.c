/**
 * This file implements minimal bindings to be compatible with the SGX-Step
 * attack framework. See also <https://github.com/jovanbulck/sgx-step>.
 */
#include "baresgx/urts.h"

extern void baresgx_default_aep(void);
extern void baresgx_aep_resume_assembly(void);
void *baresgx_aep_pointer = baresgx_default_aep;
extern uint64_t g_encl_tcs;

int aep_counter = 0;

void* sgx_get_aep(void)
{
    return baresgx_aep_pointer;
}

void sgx_set_aep(void* aep)
{
    baresgx_aep_pointer = aep;
}

void *sgx_get_tcs(void)
{
    return (void*) g_encl_tcs;
}

void c_aep_handler(void)
{
    baresgx_debug("--- Asynchronous Exit detected! Running C handler. ---");
    aep_counter++;
    return;
}

int get_aep_counter(void)
{
    return aep_counter;
}

void reset_aep_counter(void)
{
    aep_counter = 0;
}

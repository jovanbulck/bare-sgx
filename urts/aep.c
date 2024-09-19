/**
 * This file implements minimal bindings to be compatible with the SGX-Step
 * attack framework. See also <https://github.com/jovanbulck/sgx-step>.
 */
#include "baresgx/urts.h"

extern void baresgx_default_aep(void);
void *baresgx_aep_pointer = baresgx_default_aep;
void *baresgx_tcs = NULL;

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
    return baresgx_tcs;
}

#include <stdio.h>
#include "baresgx/urts.h"

#define ENCLAVE_SGXS      "enclave/encl.sgxs"
#define ENCLAVE_SIG       "enclave/encl.sig"
#define ENCLAVE_DEBUG     0

void wait_keypress(void)
{
    pid_t pid = getpid();
    printf("Press any key to continue [PID=%d]..\n", pid);
    getchar();
}

int main(void)
{
    uint64_t rv;
    void *tcs = baresgx_load_sgxs_enclave(ENCLAVE_SGXS, ENCLAVE_SIG, ENCLAVE_DEBUG, /*aexnotify=*/0);
    baresgx_info("loaded enclave TCS at %p", tcs);
    //wait_keypress();

    baresgx_info("reading enclave memory..");
    printf("\tL mem at %p is %lx\n", (void*) tcs, *((uint64_t*) tcs));

    baresgx_info("calling enclave TCS..");
    rv = baresgx_enter_enclave(tcs, /*arg=*/0);
    printf("\tL enclave returned %lx\n", rv);

    return 0;
}

# bare-trts: Trusted runtime system

Statically compiled C library that provides optional (opt-in) support for trused in-enclave validation features.

## ABI sanitization assembly stub (`entry.S`)
        
- cleanses general-purpose registers as per System-V x86-64 ABI
    - currently no support for extended FPU/SSE/etc state
- supports regular EENTER-EEXIT flow
    - currently no support for ocalls
    - currently no support for exceptions
    
## Linker script (`enclave.lds`)

- support .text, .rodata, .data, .stack
- elf2sgxs will insert unmapped guard pages for stack/text/TCS
    
## Pointer validation functions for protected ELRANGE (`trts.c`)

- add a linker symbol `__enclave_base` at the start of the enclave binary
- reserve space for `__enclave_size` to be filled in by the untrusted loader when determining the size of the final enclave image (depending on  allocated TCS/SSA etc.). The final value for `__enclave_size` is filled in  before actual enclave loading and will be measured as part of MRENCLAVE,allowing to trust the size within the enclave validation logic. This approach is similar to how this is done in real-world enclave runtimes (e.g., Intel SGX-SDK).

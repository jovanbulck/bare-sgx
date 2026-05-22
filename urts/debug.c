#include <stdio.h>
#include "baresgx/log.h"
#include "internal/sgx-defs.h"

void dump_hex(uint8_t *buf, int len)
{
    for (int i=0; i < len; i++)
        printf("%02x", *(buf + i));
    printf("\n");
}

void print_sgx_prot(unsigned int flags) {
    printf("%c%c%c",
        (flags & SGX_SECINFO_R) ? 'r' : '-',
        (flags & SGX_SECINFO_W) ? 'w' : '-',
        (flags & SGX_SECINFO_X) ? 'x' : '-');
}

void print_sgx_flags(unsigned int flags) {
    if (flags & SGX_SECINFO_SECS) {
        printf("SECS");
    } else if (flags & SGX_SECINFO_TCS) {
        printf("TCS");
    } else if (flags & SGX_SECINFO_REG) {
        printf("REG");
    } else if (flags & SGX_SECINFO_VA) {
        printf("VA");
    } else if (flags & SGX_SECINFO_TRIM) {
        printf("TRIM");
    } else {
        printf("Unknown");
    }
}

void print_sgx_secs(const struct sgx_secs *secs) {
    printf("    sgx_secs {\n");
    printf("        size: %llu\n", (unsigned long long)secs->size);
    printf("        base: %llx\n", (unsigned long long)secs->base);
    printf("        ssa_frame_size: %u\n", secs->ssa_frame_size);
    printf("        miscselect: %u\n", secs->miscselect);
    printf("        attributes: %llu\n", (unsigned long long)secs->attributes);
    printf("        xfrm: %llu\n", (unsigned long long)secs->xfrm);
    printf("        mrenclave: [");
    for (int i = 0; i < 8; i++) {
        printf("%u%s", secs->mrenclave[i], (i < 7) ? ", " : "");
    }
    printf("]\n");
    printf("        mrsigner: [");
    for (int i = 0; i < 8; i++) {
        printf("%u%s", secs->mrsigner[i], (i < 7) ? ", " : "");
    }
    printf("]\n");
    printf("        config_id: [");
    for (int i = 0; i < 16; i++) {
        printf("%u%s", secs->config_id[i], (i < 15) ? ", " : "");
    }
    printf("]\n");
    printf("        isv_prod_id: %u\n", secs->isv_prod_id);
    printf("        isv_svn: %u\n", secs->isv_svn);
    printf("        config_svn: %u\n", secs->config_svn);
    printf("    }\n");
}

void print_sgx_sigstruct_header(const struct sgx_sigstruct_header *header) {
    printf("    sgx_sigstruct_header {\n");
    printf("        header1: [%llu, %llu]\n", (unsigned long long)header->header1[0], (unsigned long long)header->header1[1]);
    printf("        vendor: 0x%04x\n", header->vendor);
    printf("        date: 0x%08x\n", header->date);
    printf("        header2: [%llu, %llu]\n", (unsigned long long)header->header2[0], (unsigned long long)header->header2[1]);
    printf("        swdefined: 0x%08x\n", header->swdefined);
    printf("        reserved1: [");
    for (int i = 0; i < 84; i++) {
        printf("%02x%s", header->reserved1[i], (i < 83) ? ", " : "");
    }
    printf("]\n");
    printf("    }\n");
}

void print_sgx_sigstruct_body(const struct sgx_sigstruct_body *body) {
    printf("    sgx_sigstruct_body {\n");
    printf("        miscselect: 0x%08x\n", body->miscselect);
    printf("        misc_mask: 0x%08x\n", body->misc_mask);
    printf("        reserved2: [");
    for (int i = 0; i < 20; i++) {
        printf("%02x%s", body->reserved2[i], (i < 19) ? ", " : "");
    }
    printf("]\n");
    printf("        attributes: 0x%016llx\n", (unsigned long long)body->attributes);
    printf("        xfrm: 0x%016llx\n", (unsigned long long)body->xfrm);
    printf("        attributes_mask: 0x%016llx\n", (unsigned long long)body->attributes_mask);
    printf("        xfrm_mask: 0x%016llx\n", (unsigned long long)body->xfrm_mask);
    printf("        mrenclave: [");
    for (int i = 0; i < 32; i++) {
        printf("%02x%s", body->mrenclave[i], (i < 31) ? ", " : "");
    }
    printf("]\n");
    printf("        reserved3: [");
    for (int i = 0; i < 32; i++) {
        printf("%02x%s", body->reserved3[i], (i < 31) ? ", " : "");
    }
    printf("]\n");
    printf("        isvprodid: 0x%04x\n", body->isvprodid);
    printf("        isvsvn: 0x%04x\n", body->isvsvn);
    printf("    }\n");
}

void print_sgx_sigstruct(const struct sgx_sigstruct *sigstruct) {
    printf("sgx_sigstruct {\n");
    print_sgx_sigstruct_header(&sigstruct->header);
    printf("    modulus: [");
    for (int i = 0; i < SGX_MODULUS_SIZE; i++) {
        printf("%02x%s", sigstruct->modulus[i], (i < SGX_MODULUS_SIZE - 1) ? ", " : "");
    }
    printf("]\n");
    printf("    exponent: 0x%08x\n", sigstruct->exponent);
    printf("    signature: [");
    for (int i = 0; i < SGX_MODULUS_SIZE; i++) {
        printf("%02x%s", sigstruct->signature[i], (i < SGX_MODULUS_SIZE - 1) ? ", " : "");
    }
    printf("]\n");
    print_sgx_sigstruct_body(&sigstruct->body);
    printf("    reserved4: [");
    for (int i = 0; i < 12; i++) {
        printf("%02x%s", sigstruct->reserved4[i], (i < 11) ? ", " : "");
    }
    printf("]\n");
    printf("    q1: [");
    for (int i = 0; i < SGX_MODULUS_SIZE; i++) {
        printf("%02x%s", sigstruct->q1[i], (i < SGX_MODULUS_SIZE - 1) ? ", " : "");
    }
    printf("]\n");
    printf("    q2: [");
    for (int i = 0; i < SGX_MODULUS_SIZE; i++) {
        printf("%02x%s", sigstruct->q2[i], (i < SGX_MODULUS_SIZE - 1) ? ", " : "");
    }
    printf("]\n");
    printf("}\n");
}

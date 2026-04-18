#include "baresgx/trts.h"

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = s1, *b = s2;
    while (n--) {
        if (*a != *b) return *a - *b;
        a++; b++;
    }
    return 0;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--)
        *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

int sgx_is_outside_enclave(void *addr, size_t len)
{
	/* need cast since void pointer arithmetics are undefined in C */
	uint64_t start = (uint64_t) addr;
	uint64_t end = start + len - 1;
	uint64_t enclave_end = get_enclave_base() + get_enclave_size();

	/* check for integer overflow with untrusted length */
	if (start > end)
		return 0;

	return (start > enclave_end) || (end < get_enclave_base());
}

int sgx_is_within_enclave(void *addr, size_t len)
{
	/* need cast since void pointer arithmetics are undefined in C */
	uint64_t start = (uint64_t) addr;
	uint64_t end = start + len - 1;
	uint64_t enclave_end = get_enclave_base() + get_enclave_size();

	/* check for integer overflow with untrusted length */
	if (start > end)
		return 0;

	return (start >= get_enclave_base() && end <= enclave_end);
}

void panic(void)
{
	asm("ud2");
}

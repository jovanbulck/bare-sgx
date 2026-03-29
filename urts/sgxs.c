#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "baresgx/urts.h"
#include "internal/sgx-defs.h"
#include "internal/sgxs.h"

#define EXIT_SGXS_NON_CANONICAL(msg, ...)					   \
    do {                                                       \
        baresgx_error("non-canonical SGXS stream '%s': " msg,  \
			          g_sgxs_path, ##__VA_ARGS__);             \
        exit(1);                                               \
    } while(0)

#define ASSERT_SGXS_CANONICAL(cond, msg, ...)                  \
    do {                                                       \
        if (!(cond)) {                                         \
			EXIT_SGXS_NON_CANONICAL(msg, ##__VA_ARGS__);       \
        }                                                      \
    } while(0)

const char *g_sgxs_path = "";
int g_fd_dev = 0, g_ecreated = 0;
uint64_t g_encl_base = 0;
uint64_t g_encl_size = 0;
uint64_t g_encl_tcs = 0;

static uint64_t map_enclave_area(uint64_t encl_size)
{
	void *area;
	uint64_t base;

	/* Intel SDM: Enclave Base Linear Address must be naturally aligned to size. */
	BARESGX_ASSERT((area = mmap(NULL, encl_size * 2, PROT_NONE,
								MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) != MAP_FAILED);
	base = ((uint64_t)area + encl_size - 1) & ~(encl_size - 1);

	munmap(area, base - (uint64_t)area);
	munmap((void *)(base + encl_size), (uint64_t)area + encl_size - base);

	return base;
}

static int ioc_ecreate(int debug, uint8_t *sgxs, int sgxs_offset)
{
	struct sgx_enclave_create ioc_create = {0};
	struct sgxs_ecreate *ecreate;
	struct sgx_secs secs = {0};

	ASSERT_SGXS_CANONICAL(!g_ecreated, "only one ECREATE");
	ecreate = (struct sgxs_ecreate *)(sgxs + sgxs_offset);
	g_encl_base = map_enclave_area(ecreate->size);
	baresgx_debug("ECREATE    : size=%#lx; ssaframesize=%u; base=%#lx",
		ecreate->size, ecreate->ssaframesize, g_encl_base);
	
	secs.ssa_frame_size = ecreate->ssaframesize;
	secs.attributes = SGX_ATTR_MODE64BIT;
	secs.attributes |= debug ? SGX_ATTR_DEBUG : 0x0;
	secs.xfrm = 3;
	secs.base = g_encl_base;
	secs.size = ecreate->size;
	g_encl_size = ecreate->size;
	
	ioc_create.src = (unsigned long)&secs;
	BARESGX_ASSERT(ioctl(g_fd_dev, SGX_IOC_ENCLAVE_CREATE, &ioc_create) >= 0);
	sgxs_offset += sizeof(struct sgxs_ecreate);

	g_ecreated = 1;
	return sgxs_offset;
}

static int ioc_eadd(uint8_t *sgxs, int sgxs_offset)
{
	struct sgx_enclave_add_pages ioc_add = {0};
	uint8_t *page_data;
	struct sgx_secinfo secinfo = {0};
	struct sgxs_eextend *eextend;
	int nb_eextend, eextend_idx;
	struct sgxs_eadd *eadd;

	eadd = (struct sgxs_eadd *)(sgxs + sgxs_offset);
	baresgx_debug("EADD       : offset=%#lx; flags=%#lx", eadd->offset, eadd->flags);

	ASSERT_SGXS_CANONICAL(g_ecreated, "must start with ECREATE");
	ASSERT_SGXS_CANONICAL(!(eadd->offset & (PAGE_SIZE-1)), "EADD must be page-aligned");

	/* Driver requires page-aligned memory */
	BARESGX_ASSERT((page_data = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
									 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) != MAP_FAILED);
	secinfo.flags = eadd->flags;
    ioc_add.src = (unsigned long) page_data;
	ioc_add.offset = eadd->offset;
	ioc_add.length = PAGE_SIZE;
	ioc_add.secinfo = (unsigned long) &secinfo;

	if (eadd->flags & SGX_SECINFO_TCS) {
		ASSERT_SGXS_CANONICAL(!(eadd->flags & SGX_SECINFO_R), "TCS permissions must be cleared");
		ASSERT_SGXS_CANONICAL(!(eadd->flags & SGX_SECINFO_W), "TCS permissions must be cleared");
		ASSERT_SGXS_CANONICAL(!(eadd->flags & SGX_SECINFO_X), "TCS permissions must be cleared");
		g_encl_tcs = g_encl_base + eadd->offset;
	}

	/* Copy data: canonical SGXS stream is followed by 0 or 16 EEXTEND records */
	nb_eextend = 0;
	sgxs_offset += sizeof(struct sgxs_eadd);
	while (*((uint64_t *)(sgxs + sgxs_offset)) == SGXS_TAG_EEXTEND)
	{
		nb_eextend++;
		ioc_add.flags |= SGX_PAGE_MEASURE;
		eextend = (struct sgxs_eextend *)(sgxs + sgxs_offset);
		baresgx_debug(" |_ EEXTEND: offset=%#lx", eextend->offset);

		eextend_idx = eextend->offset - eadd->offset;
		ASSERT_SGXS_CANONICAL(eextend_idx < PAGE_SIZE, "unordered EEXTEND");
		ASSERT_SGXS_CANONICAL(!(eextend->offset & (EEXTEND_SIZE-1)), "EEXTEND alignment");

		memcpy(page_data + eextend_idx, eextend->blob, EEXTEND_SIZE);
		sgxs_offset += sizeof(struct sgxs_eextend);
	}
	ASSERT_SGXS_CANONICAL(nb_eextend == 0 || nb_eextend == (PAGE_SIZE / EEXTEND_SIZE),
		"EADD followed by %d EEXTEND records (expect 0 or 16)", nb_eextend);
	
	BARESGX_ASSERT(ioctl(g_fd_dev, SGX_IOC_ENCLAVE_ADD_PAGES, &ioc_add) >= 0);
	BARESGX_ASSERT(ioc_add.count == PAGE_SIZE);

	/* create mapping in untrusted page table with capped permission */
	int prot = 0;
	prot |= (eadd->flags & SGX_SECINFO_R)   ? PROT_READ  : 0x0;
	prot |= (eadd->flags & SGX_SECINFO_W)   ? PROT_WRITE : 0x0;
	prot |= (eadd->flags & SGX_SECINFO_X)   ? PROT_EXEC  : 0x0;
	prot |= (eadd->flags & SGX_SECINFO_TCS) ? PROT_READ | PROT_WRITE : 0x0;
	BARESGX_ASSERT(mmap((void*)g_encl_base + ioc_add.offset, PAGE_SIZE, prot,
						 MAP_SHARED | MAP_FIXED, g_fd_dev, 0) != MAP_FAILED);

	munmap(page_data, PAGE_SIZE);
	return sgxs_offset;
}

static void ioc_einit(struct sgx_sigstruct *sigstruct)
{
	struct sgx_enclave_init ioc_einit = {0};

	ioc_einit.sigstruct = (uint64_t) sigstruct;
	BARESGX_ASSERT(ioctl(g_fd_dev, SGX_IOC_ENCLAVE_INIT, &ioc_einit) >= 0);
}

void* baresgx_load_sgxs_enclave(const char *sgxs_path, const char * sigstruct_path, int debug)
{
	int fd_sgxs, fd_sig, offset = 0;
    uint8_t *sgxs = NULL;
    struct stat sb;
    uint64_t tag;
	struct sgx_sigstruct *sigstruct;

	/* open SGX driver */
	BARESGX_ASSERT((g_fd_dev = open("/dev/sgx_enclave", O_RDWR)) > 0);

    /* mmap SGXS input file */
    BARESGX_ASSERT((fd_sgxs = open(sgxs_path, O_RDONLY)) > 0);
    BARESGX_ASSERT(fstat(fd_sgxs, &sb) >= 0);
    BARESGX_ASSERT((sgxs = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd_sgxs, 0)) != MAP_FAILED);

	/* parse SGXS tags one by one and translate into IOCTL system calls */
	g_sgxs_path = sgxs_path;
	while (offset < sb.st_size)
	{
		switch (tag = *((uint64_t *)(sgxs + offset)))
		{
			case SGXS_TAG_ECREATE:
				offset = ioc_ecreate(debug, sgxs, offset);
				break;
			case SGXS_TAG_EADD:
				offset = ioc_eadd(sgxs, offset);
				break;
			case SGXS_TAG_EEXTEND:
				EXIT_SGXS_NON_CANONICAL("Unexpected EEXTEND tag at offset %#x", offset);
			default:
				EXIT_SGXS_NON_CANONICAL("Unknown SGXS tag %#lx at offset %#x", tag, offset);
		}
	}

	/* finalize signed enclave */
    BARESGX_ASSERT((fd_sig = open(sigstruct_path, O_RDONLY)) > 0);
    BARESGX_ASSERT((void*)(sigstruct = mmap(NULL, sizeof(struct sgx_sigstruct), PROT_READ,
											MAP_PRIVATE, fd_sig, 0)) != MAP_FAILED);
	ioc_einit(sigstruct);

	/* cleanup */
	munmap(sgxs, sb.st_size);
	munmap(sigstruct, sizeof(struct sgx_sigstruct));
	close(fd_sgxs);
	close(fd_sig);
	close(g_fd_dev);
	
	return (void*) g_encl_tcs;
}
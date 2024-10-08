// SPDX-License-Identifier: GPL-2.0
/*  Copyright(c) 2016-20 Intel Corporation. */

#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "baresgx/urts.h"
#include "internal/elf-enclave.h"

extern void *baresgx_tcs;

void encl_delete(struct encl *encl)
{
	struct encl_segment *heap_seg;

	if (encl->encl_base)
		munmap((void *)encl->encl_base, encl->encl_size);

	if (encl->bin)
		munmap(encl->bin, encl->bin_size);

	if (encl->fd)
		close(encl->fd);

	if (encl->segment_tbl) {
		heap_seg = &encl->segment_tbl[encl->nr_segments - 1];
		munmap(heap_seg->src, heap_seg->size);
		free(encl->segment_tbl);
	}

	memset(encl, 0, sizeof(*encl));
}

static bool encl_map_bin(const char *path, struct encl *encl)
{
	struct stat sb;
	void *bin;
	int ret;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd == -1)  {
		perror("enclave executable open()");
		return false;
	}

	ret = stat(path, &sb);
	if (ret) {
		perror("enclave executable stat()");
		goto err;
	}

	bin = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (bin == MAP_FAILED) {
		perror("enclave executable mmap()");
		goto err;
	}

	encl->bin = bin;
	encl->bin_size = sb.st_size;

	close(fd);
	return true;

err:
	close(fd);
	return false;
}

static bool encl_ioc_create(struct encl *encl, int debug)
{
	struct sgx_secs *secs = &encl->secs;
	struct sgx_enclave_create ioc;
	int rc;

	assert(encl->encl_base != 0);

	memset(secs, 0, sizeof(*secs));
	secs->ssa_frame_size = 1;
	secs->attributes = SGX_ATTR_MODE64BIT;
	if (debug)
        	secs->attributes |= SGX_ATTR_DEBUG;
	secs->xfrm = 3;
	secs->base = encl->encl_base;
	secs->size = encl->encl_size;

	ioc.src = (unsigned long)secs;
	rc = ioctl(encl->fd, SGX_IOC_ENCLAVE_CREATE, &ioc);
	if (rc) {
		perror("SGX_IOC_ENCLAVE_CREATE failed");
		munmap((void *)secs->base, encl->encl_size);
		return false;
	}

	return true;
}

static bool encl_ioc_add_pages(struct encl *encl, struct encl_segment *seg)
{
	struct sgx_enclave_add_pages ioc;
	struct sgx_secinfo secinfo;
	int rc;

	memset(&secinfo, 0, sizeof(secinfo));
	secinfo.flags = seg->flags;

	ioc.src = (uint64_t)seg->src;
	ioc.offset = seg->offset;
	ioc.length = seg->size;
	ioc.secinfo = (unsigned long)&secinfo;
	if (seg->measure)
		ioc.flags = SGX_PAGE_MEASURE;
	else
		ioc.flags = 0;

	rc = ioctl(encl->fd, SGX_IOC_ENCLAVE_ADD_PAGES, &ioc);
	if (rc < 0) {
		perror("SGX_IOC_ENCLAVE_ADD_PAGES failed");
		return false;
	}

	return true;
}

/*
 * Parse the enclave code's symbol table to locate and return address of
 * the provided symbol
 */
uint64_t encl_get_entry(struct encl *encl, const char *symbol)
{
	Elf64_Sym *symtab = NULL;
	char *sym_names = NULL;
	Elf64_Shdr *sections;
	Elf64_Ehdr *ehdr;
	int num_sym = 0;
	int i;

	ehdr = encl->bin;
	sections = encl->bin + ehdr->e_shoff;

	for (i = 0; i < ehdr->e_shnum; i++) {
		if (sections[i].sh_type == SHT_SYMTAB) {
			symtab = (Elf64_Sym *)((char *)encl->bin + sections[i].sh_offset);
			num_sym = sections[i].sh_size / sections[i].sh_entsize;
			break;
		}
	}

	for (i = 0; i < ehdr->e_shnum; i++) {
		if (sections[i].sh_type == SHT_STRTAB) {
			sym_names = (char *)encl->bin + sections[i].sh_offset;
			break;
		}
	}

	if (!symtab || !sym_names)
		return 0;

	for (i = 0; i < num_sym; i++) {
		Elf64_Sym *sym = &symtab[i];

		if (!strcmp(symbol, sym_names + sym->st_name))
			return (uint64_t)sym->st_value;
	}

	return 0;
}

bool encl_load(const char *path, struct encl *encl, unsigned long heap_size)
{
	const char device_path[] = "/dev/sgx_enclave";
	struct encl_segment *seg;
	Elf64_Phdr *phdr_tbl;
	off_t src_offset;
	Elf64_Ehdr *ehdr;
	struct stat sb;
	void *ptr;
	int i, j;
	int ret;
	int fd = -1;

	memset(encl, 0, sizeof(*encl));

	fd = open(device_path, O_RDWR);
	if (fd < 0) {
		perror("Unable to open /dev/sgx_enclave");
		goto err;
	}

	ret = stat(device_path, &sb);
	if (ret) {
		perror("device file stat()");
		goto err;
	}

	ptr = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, 0);
	if (ptr == (void *)-1) {
		perror("mmap for read");
		goto err;
	}
	munmap(ptr, PAGE_SIZE);

#define ERR_MSG \
"mmap() succeeded for PROT_READ, but failed for PROT_EXEC.\n" \
" Check that /dev does not have noexec set:\n" \
" \tmount | grep \"/dev .*noexec\"\n" \
" If so, remount it executable: mount -o remount,exec /dev\n\n"

	ptr = mmap(NULL, PAGE_SIZE, PROT_EXEC, MAP_SHARED, fd, 0);
	if (ptr == (void *)-1) {
		fprintf(stderr, ERR_MSG);
		goto err;
	}
	munmap(ptr, PAGE_SIZE);

	encl->fd = fd;

	if (!encl_map_bin(path, encl))
		goto err;

	ehdr = encl->bin;
	phdr_tbl = encl->bin + ehdr->e_phoff;

	encl->nr_segments = 1; /* one for the heap */

	for (i = 0; i < ehdr->e_phnum; i++) {
		Elf64_Phdr *phdr = &phdr_tbl[i];

		if (phdr->p_type == PT_LOAD)
			encl->nr_segments++;
	}

	encl->segment_tbl = calloc(encl->nr_segments,
				   sizeof(struct encl_segment));
	if (!encl->segment_tbl)
		goto err;

	for (i = 0, j = 0; i < ehdr->e_phnum; i++) {
		Elf64_Phdr *phdr = &phdr_tbl[i];
		unsigned int flags = phdr->p_flags;

		if (phdr->p_type != PT_LOAD)
			continue;

		seg = &encl->segment_tbl[j];

		if (!!(flags & ~(PF_R | PF_W | PF_X))) {
			fprintf(stderr,
				"%d has invalid segment flags 0x%02x.\n", i,
				phdr->p_flags);
			goto err;
		}

		if (j == 0 && flags != (PF_R | PF_W)) {
			fprintf(stderr,
				"TCS has invalid segment flags 0x%02x.\n",
				phdr->p_flags);
			goto err;
		}

		if (j == 0) {
			src_offset = phdr->p_offset & PAGE_MASK;
			encl->src = encl->bin + src_offset;

			seg->prot = PROT_READ | PROT_WRITE;
			seg->flags = SGX_PAGE_TYPE_TCS << 8;
		} else  {
			seg->prot = (phdr->p_flags & PF_R) ? PROT_READ : 0;
			seg->prot |= (phdr->p_flags & PF_W) ? PROT_WRITE : 0;
			seg->prot |= (phdr->p_flags & PF_X) ? PROT_EXEC : 0;
			seg->flags = (SGX_PAGE_TYPE_REG << 8) | seg->prot;
		}

		seg->offset = (phdr->p_offset & PAGE_MASK) - src_offset;
		seg->size = (phdr->p_filesz + PAGE_SIZE - 1) & PAGE_MASK;
		seg->src = encl->src + seg->offset;
		seg->measure = true;

		j++;
	}

	assert(j == encl->nr_segments - 1);

	seg = &encl->segment_tbl[j];
	seg->offset =  encl->segment_tbl[j - 1].offset + encl->segment_tbl[j - 1].size;
	seg->size = heap_size;
	seg->src = mmap(NULL, heap_size, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	seg->prot = PROT_READ | PROT_WRITE;
	seg->flags = (SGX_PAGE_TYPE_REG << 8) | seg->prot;
	seg->measure = false;

	if (seg->src == MAP_FAILED)
		goto err;

	encl->src_size = encl->segment_tbl[j].offset + encl->segment_tbl[j].size;

	for (encl->encl_size = 4096; encl->encl_size < encl->src_size; )
		encl->encl_size <<= 1;

	return true;

err:
	if (fd != -1)
		close(fd);
	encl_delete(encl);
	return false;
}

static bool encl_map_area(struct encl *encl)
{
	size_t encl_size = encl->encl_size;
	void *area;

	area = mmap(NULL, encl_size * 2, PROT_NONE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (area == MAP_FAILED) {
		perror("reservation mmap()");
		return false;
	}

	encl->encl_base = ((uint64_t)area + encl_size - 1) & ~(encl_size - 1);

	munmap(area, encl->encl_base - (uint64_t)area);
	munmap((void *)(encl->encl_base + encl_size),
	       (uint64_t)area + encl_size - encl->encl_base);

	return true;
}

bool encl_build(struct encl *encl, int debug)
{
	struct sgx_enclave_init ioc;
	int ret;
	int i;

	if (!encl_map_area(encl))
		return false;

	if (!encl_ioc_create(encl, debug))
		return false;

	/*
	 * Pages must be added before mapping VMAs because their permissions
	 * cap the VMA permissions.
	 */
	for (i = 0; i < encl->nr_segments; i++) {
		struct encl_segment *seg = &encl->segment_tbl[i];

		if (!encl_ioc_add_pages(encl, seg))
			return false;
	}

	ioc.sigstruct = (uint64_t)&encl->sigstruct;
	ret = ioctl(encl->fd, SGX_IOC_ENCLAVE_INIT, &ioc);
	if (ret) {
		perror("SGX_IOC_ENCLAVE_INIT failed");
		return false;
	}

	return true;
}

/*
 * Return the offset in the enclave where the TCS segment can be found.
 * The first RW segment loaded is the TCS.
 */
static off_t encl_get_tcs_offset(struct encl *encl)
{
	int i;

	for (i = 0; i < encl->nr_segments; i++) {
		struct encl_segment *seg = &encl->segment_tbl[i];

		if (i == 0 && seg->prot == (PROT_READ | PROT_WRITE))
			return seg->offset;
	}

	return -1;
}

void* baresgx_load_elf_enclave(const char *path, int debug)
{
    struct encl encl;
    int i;

    baresgx_debug("parsing enclave binary '%s'..", path);
    BARESGX_ASSERT( encl_load(path, &encl, /*heap_size=*/PAGE_SIZE) );

    baresgx_debug("measuring enclave binary..");
    BARESGX_ASSERT( encl_measure(&encl) );

    baresgx_debug("loading enclave binary..");
    BARESGX_ASSERT( encl_build(&encl, debug) );

    baresgx_debug("mmapping loaded enclave..");
    for (i = 0; i < encl.nr_segments; i++) {
    	struct encl_segment *seg = &encl.segment_tbl[i];
    
    	BARESGX_ASSERT( mmap((void *)encl.encl_base + seg->offset, seg->size,
    		    seg->prot, MAP_SHARED | MAP_FIXED, encl.fd, 0) != MAP_FAILED);
    }

    #if BARESGX_DEBUG
        pretty_print_encl(&encl);
    #endif

    baresgx_tcs = (void*) (encl.encl_base + encl_get_tcs_offset(&encl));
    return baresgx_tcs;
}

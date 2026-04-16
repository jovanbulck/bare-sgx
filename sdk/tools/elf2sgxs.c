#include <stdio.h>
#include <string.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include "internal/sgx-defs.h"
#include "internal/sgxs.h"
#include "baresgx/urts.h"

int g_elf_fd = 0, g_sgxs_fd = 0;
size_t g_sgxs_offset = 0, g_sgxs_elf_offset = 0;
static unsigned int g_ssa_frame_size = 1, g_nssa = 1;
static const char *g_entry_symbol = "encl_entry";

#define ELF_MAGIC 0x464C457FU

static void sgxs_append(const void *buf, size_t size)
{
    BARESGX_ASSERT(write(g_sgxs_fd, buf, size) == (ssize_t)size);
}

static void sgxs_ecreate(size_t size)
{
    struct sgxs_ecreate ecreate = {0};
    uint64_t enclave_size = 1;

    /* align to nearest power of 2 (required by ECREATE) */
    while (enclave_size < size * PAGE_SIZE)
        enclave_size <<= 1;

    ecreate.tag = SGXS_TAG_ECREATE;
    ecreate.ssaframesize = g_ssa_frame_size;
    ecreate.size = enclave_size;
    sgxs_append(&ecreate, sizeof(ecreate));
}

static void sgxs_add_guard_page(void)
{
    g_sgxs_offset &= PAGE_MASK;
    g_sgxs_offset += PAGE_SIZE;
}
    
static void sgxs_add_page(const void* data, uint64_t secinfo_flags, int measure)
{
    struct sgxs_eadd eadd = {0x00};
    struct sgxs_eextend eextend = {0x00};
    int i;

    eadd.tag = SGXS_TAG_EADD;
    eadd.offset = g_sgxs_offset;
    eadd.flags = secinfo_flags;
    sgxs_append(&eadd, sizeof(eadd));

    if (measure) {
        for (i = 0; i < PAGE_SIZE; i += EEXTEND_SIZE) {
            eextend.tag = SGXS_TAG_EEXTEND;
            eextend.offset = g_sgxs_offset + i;
            memcpy(eextend.blob, data + i, EEXTEND_SIZE);
            sgxs_append(&eextend, sizeof(eextend));
        }
    }

    g_sgxs_offset += PAGE_SIZE;
}

static uint64_t elf_flags_to_sgx(uint32_t flags)
{
    return ((flags & PF_R) ? SGX_SECINFO_R : 0) |
           ((flags & PF_W) ? SGX_SECINFO_W : 0) |
           ((flags & PF_X) ? SGX_SECINFO_X : 0) |
                             SGX_SECINFO_REG;
}

static uint64_t elf_symbol(const char *symname, uint8_t *elf)
{
    Elf64_Ehdr *ehdr  = (Elf64_Ehdr *)elf;
    Elf64_Shdr *shdrs = (Elf64_Shdr *)(elf + ehdr->e_shoff);

    for (int i = 0; i < ehdr->e_shnum; i++) {
        if (shdrs[i].sh_type != SHT_SYMTAB)
            continue;

        Elf64_Sym  *syms    = (Elf64_Sym *)(elf + shdrs[i].sh_offset);
        const char *strtab  = (char *)     (elf + shdrs[shdrs[i].sh_link].sh_offset);
        int         nsyms   = shdrs[i].sh_size / sizeof(Elf64_Sym);

        for (int j = 0; j < nsyms; j++)
            if (strcmp(strtab + syms[j].st_name, symname) == 0)
                return g_sgxs_elf_offset + syms[j].st_value;
    }

    BARESGX_ASSERT(0 && "symbol not found");
}

static const char *elf_addr2name(uint64_t addr, uint8_t *elf)
{
    Elf64_Ehdr *ehdr     = (Elf64_Ehdr *)(elf);
    Elf64_Shdr *shdrs    = (Elf64_Shdr *)(elf + ehdr->e_shoff);
    const char *shstrtab = (char *)      (elf + shdrs[ehdr->e_shstrndx].sh_offset);
    const char *name;

    for (int i = 0; i < ehdr->e_shnum; i++)
        if (shdrs[i].sh_addr == addr) {
            name = shstrtab + shdrs[i].sh_name;
            if (*name) return name;
        }
    return "?";
}

static int parse_args(int argc, char *argv[])
{
    int positional = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--ssaframesize")) {
            BARESGX_ASSERT_RET((i+1) < argc, "ssaframesize");
            g_ssa_frame_size = atoi(argv[++i]);
            BARESGX_ASSERT_RET(g_ssa_frame_size > 0, "ssaframesize");
        } else if (!strcmp(argv[i], "--nssa")) {
            BARESGX_ASSERT_RET((i+1) < argc, "nssa");
            g_nssa = atoi(argv[++i]);
            BARESGX_ASSERT_RET(g_nssa > 0, "nssa");
        } else if (!strcmp(argv[i], "--entry")) {
            BARESGX_ASSERT_RET((i+1) < argc, "entry");
            g_entry_symbol = argv[++i];
        } else if (argv[i][0] == '-') {
            BARESGX_ASSERT_RET(0, argv[i]);
        } else {
            switch (positional++) {
                case 0:
                    g_elf_fd = open(argv[i], O_RDONLY);
                    BARESGX_ASSERT_RET(g_elf_fd > 0, argv[i]);
                    break;
                case 1:
                    g_sgxs_fd = open(argv[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    BARESGX_ASSERT_RET(g_sgxs_fd > 0, argv[i]);
                    break;
                default:
                    BARESGX_ASSERT_RET(0, argv[i]);
            }
        }
    }

    return (positional < 2) ? -1 : 0;
}

int main(int argc, char *argv[])
{
    int i, offset, measure, elf_pages;
    uint8_t pagebuf[PAGE_SIZE];
    uint8_t *elf, *data;
    struct sgx_tcs tcs = {0};
    Elf64_Phdr *phdrs;
    Elf64_Ehdr *ehdr;
    const char *name;
    struct stat sb;
    uint64_t flags;
    
    if (parse_args(argc, argv) < 0) {
        fprintf(stderr, "Usage: %s [--ssaframesize=N] [--nssa=N] [--entry=symbol_name] <input_elf> <output_sgxs>\n", argv[0]);
        return -1;
    }

    /* mmap valid input ELF file */
    BARESGX_ASSERT(fstat(g_elf_fd, &sb) >= 0);
    BARESGX_ASSERT((elf = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, g_elf_fd, 0)) != MAP_FAILED);
    BARESGX_ASSERT( (*((uint32_t*) elf) == ELF_MAGIC));

    /* parse input ELF file: extract program header */
    ehdr     = (Elf64_Ehdr *) (elf);
    phdrs    = (Elf64_Phdr *) (elf + ehdr->e_phoff);

    /* compute required enclave size and create new output SGXS stream */
    for (i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD)
            continue;;
        elf_pages += (phdrs[i].p_memsz + PAGE_SIZE - 1) & PAGE_MASK;
    }
    sgxs_ecreate(elf_pages + g_nssa*PAGE_SIZE + /*tcs*/ 1*PAGE_SIZE + /*guard pages*/ 2*PAGE_SIZE);
    sgxs_add_guard_page();
    g_sgxs_elf_offset = g_sgxs_offset;

    /* write each loadable ELF segment into the output SGXS stream */
    for (i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD || phdrs[i].p_memsz <= 0)
            continue;

        name  = elf_addr2name(phdrs[i].p_vaddr, elf);
        flags = elf_flags_to_sgx(phdrs[i].p_flags);
        data  = elf + phdrs[i].p_offset;
        measure = strcmp(name, ".noinit") == 0 ? 0 : 1;

        baresgx_info("adding section '%-5.5s': addr=0x%04lx size=0x%04lx flags=%s",
               name, phdrs[i].p_vaddr, phdrs[i].p_memsz,
               sgx_secinfo_flags_to_str(flags));

        for (offset = 0; offset < phdrs[i].p_memsz; offset += PAGE_SIZE) {
            memset(pagebuf, 0x00, PAGE_SIZE);
            if (offset < phdrs[i].p_filesz)
                memcpy(pagebuf, data + offset, MIN(PAGE_SIZE, phdrs[i].p_filesz - offset));
            sgxs_add_page(pagebuf, flags, measure);
        }
    }

    /* finally add the TCS and SSA pages based on the user input */
    sgxs_add_guard_page();
    
    tcs.entry_offset = elf_symbol(g_entry_symbol, elf);
    tcs.ssa_offset = g_sgxs_offset + PAGE_SIZE; /* SSA follows immediately after TCS */
    tcs.nr_ssa_frames = g_nssa;
    tcs.fs_limit = 0xFFFFFFFF;
    tcs.gs_limit = 0xFFFFFFFF;

    sgxs_add_page(&tcs, SGX_SECINFO_TCS, /*measure=*/1);
    for (i = 0; i < g_nssa; i++)
        sgxs_add_page(NULL, SGX_SECINFO_R | SGX_SECINFO_W | SGX_SECINFO_REG, /*measure=*/0);

    return 0;
}
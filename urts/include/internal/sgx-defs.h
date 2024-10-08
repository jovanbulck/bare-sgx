#ifndef SGX_DEFS_H_INC
#define SGX_DEFS_H_INC

/*
 * Architectural definitions of SGX structures, as defined in Intel SDM and
 * copied from Linux kernel.
 */

#pragma pack(1)
#define BIT(nr)			(1UL << (nr))

/* https://elixir.bootlin.com/linux/latest/source/arch/x86/include/asm/sgx.h#L291 */
/**
 * struct sgx_secs - SGX Enclave Control Structure (SECS)
 * @size:		size of the address space
 * @base:		base address of the  address space
 * @ssa_frame_size:	size of an SSA frame
 * @miscselect:		additional information stored to an SSA frame
 * @attributes:		attributes for enclave
 * @xfrm:		XSave-Feature Request Mask (subset of XCR0)
 * @mrenclave:		SHA256-hash of the enclave contents
 * @mrsigner:		SHA256-hash of the public key used to sign the SIGSTRUCT
 * @config_id:		a user-defined value that is used in key derivation
 * @isv_prod_id:	a user-defined value that is used in key derivation
 * @isv_svn:		a user-defined value that is used in key derivation
 * @config_svn:		a user-defined value that is used in key derivation
 *
 * SGX Enclave Control Structure (SECS) is a special enclave page that is not
 * visible in the address space. In fact, this structure defines the address
 * range and other global attributes for the enclave and it is the first EPC
 * page created for any enclave. It is moved from a temporary buffer to an EPC
 * by the means of ENCLS[ECREATE] function.
 */
struct sgx_secs {
	uint64_t size;
	uint64_t base;
	uint32_t ssa_frame_size;
	uint32_t miscselect;
	uint8_t  reserved1[24];
	uint64_t attributes;
	uint64_t xfrm;
	uint32_t mrenclave[8];
	uint8_t  reserved2[32];
	uint32_t mrsigner[8];
	uint8_t  reserved3[32];
	uint32_t config_id[16];
	uint16_t isv_prod_id;
	uint16_t isv_svn;
	uint16_t config_svn;
	uint8_t  reserved4[3834];
};

/**
 * struct sgx_secinfo - describes attributes of an EPC page
 * @flags:	permissions and type
 *
 * Used together with ENCLS leaves that add or modify an EPC page to an
 * enclave to define page permissions and type.
 */
struct sgx_secinfo {
	uint64_t flags;
	uint8_t  reserved[56];
};

/**
 * struct sgx_tcs - Thread Control Structure (TCS)
 * @state:		used to mark an entered TCS
 * @flags:		execution flags (cleared by EADD)
 * @ssa_offset:		SSA stack offset relative to the enclave base
 * @ssa_index:		the current SSA frame index (cleard by EADD)
 * @nr_ssa_frames:	the number of frame in the SSA stack
 * @entry_offset:	entry point offset relative to the enclave base
 * @exit_addr:		address outside the enclave to exit on an exception or
 *			interrupt
 * @fs_offset:		offset relative to the enclave base to become FS
 *			segment inside the enclave
 * @gs_offset:		offset relative to the enclave base to become GS
 *			segment inside the enclave
 * @fs_limit:		size to become a new FS-limit (only 32-bit enclaves)
 * @gs_limit:		size to become a new GS-limit (only 32-bit enclaves)
 *
 * Thread Control Structure (TCS) is an enclave page visible in its address
 * space that defines an entry point inside the enclave. A thread enters inside
 * an enclave by supplying address of TCS to ENCLU(EENTER). A TCS can be entered
 * by only one thread at a time.
 */
struct sgx_tcs {
	uint64_t state;
	uint64_t flags;
	uint64_t ssa_offset;
	uint32_t ssa_index;
	uint32_t nr_ssa_frames;
	uint64_t entry_offset;
	uint64_t exit_addr;
	uint64_t fs_offset;
	uint64_t gs_offset;
	uint32_t fs_limit;
	uint32_t gs_limit;
	uint8_t  reserved[4024];
};

/**
 * struct sgx_sigstruct_header -  defines author of the enclave
 * @header1:		constant byte string
 * @vendor:		must be either 0x0000 or 0x8086
 * @date:		YYYYMMDD in BCD
 * @header2:		constant byte string
 * @swdefined:		software defined value
 */
struct sgx_sigstruct_header {
	uint64_t header1[2];
	uint32_t vendor;
	uint32_t date;
	uint64_t header2[2];
	uint32_t swdefined;
	uint8_t  reserved1[84];
};

/**
 * struct sgx_sigstruct_body - defines contents of the enclave
 * @miscselect:		additional information stored to an SSA frame
 * @misc_mask:		required miscselect in SECS
 * @attributes:		attributes for enclave
 * @xfrm:		XSave-Feature Request Mask (subset of XCR0)
 * @attributes_mask:	required attributes in SECS
 * @xfrm_mask:		required XFRM in SECS
 * @mrenclave:		SHA256-hash of the enclave contents
 * @isvprodid:		a user-defined value that is used in key derivation
 * @isvsvn:		a user-defined value that is used in key derivation
 */
struct sgx_sigstruct_body {
	uint32_t miscselect;
	uint32_t misc_mask;
	uint8_t  reserved2[20];
	uint64_t attributes;
	uint64_t xfrm;
	uint64_t attributes_mask;
	uint64_t xfrm_mask;
	uint8_t  mrenclave[32];
	uint8_t  reserved3[32];
	uint16_t isvprodid;
	uint16_t isvsvn;
};

/* The modulus size for 3072-bit RSA keys. */
#define SGX_MODULUS_SIZE 384

/**
 * struct sgx_sigstruct - an enclave signature
 * @header:		defines author of the enclave
 * @modulus:		the modulus of the public key
 * @exponent:		the exponent of the public key
 * @signature:		the signature calculated over the fields except modulus,
 * @body:		defines contents of the enclave
 * @q1:			a value used in RSA signature verification
 * @q2:			a value used in RSA signature verification
 *
 * Header and body are the parts that are actual signed. The remaining fields
 * define the signature of the enclave.
 */
struct sgx_sigstruct {
	struct sgx_sigstruct_header header;
	uint8_t  modulus[SGX_MODULUS_SIZE];
	uint32_t exponent;
	uint8_t  signature[SGX_MODULUS_SIZE];
	struct sgx_sigstruct_body body;
	uint8_t  reserved4[12];
	uint8_t  q1[SGX_MODULUS_SIZE];
	uint8_t  q2[SGX_MODULUS_SIZE];
};

/**
 * enum sgx_page_type - bits in the SECINFO flags defining the page type
 * %SGX_PAGE_TYPE_SECS:	a SECS page
 * %SGX_PAGE_TYPE_TCS:	a TCS page
 * %SGX_PAGE_TYPE_REG:	a regular page
 * %SGX_PAGE_TYPE_VA:	a VA page
 * %SGX_PAGE_TYPE_TRIM:	a page in trimmed state
 *
 * Make sure when making changes to this enum that its values can still fit
 * in the bitfield within &struct sgx_encl_page
 */
enum sgx_page_type {
	SGX_PAGE_TYPE_SECS,
	SGX_PAGE_TYPE_TCS,
	SGX_PAGE_TYPE_REG,
	SGX_PAGE_TYPE_VA,
	SGX_PAGE_TYPE_TRIM,
};

#define SGX_NR_PAGE_TYPES	5
#define SGX_PAGE_TYPE_MASK	GENMASK(7, 0)

/**
 * enum sgx_secinfo_flags - the flags field in &struct sgx_secinfo
 * %SGX_SECINFO_R:	allow read
 * %SGX_SECINFO_W:	allow write
 * %SGX_SECINFO_X:	allow execution
 * %SGX_SECINFO_SECS:	a SECS page
 * %SGX_SECINFO_TCS:	a TCS page
 * %SGX_SECINFO_REG:	a regular page
 * %SGX_SECINFO_VA:	a VA page
 * %SGX_SECINFO_TRIM:	a page in trimmed state
 */
enum sgx_secinfo_flags {
	SGX_SECINFO_R			= BIT(0),
	SGX_SECINFO_W			= BIT(1),
	SGX_SECINFO_X			= BIT(2),
	SGX_SECINFO_SECS		= (SGX_PAGE_TYPE_SECS << 8),
	SGX_SECINFO_TCS			= (SGX_PAGE_TYPE_TCS << 8),
	SGX_SECINFO_REG			= (SGX_PAGE_TYPE_REG << 8),
	SGX_SECINFO_VA			= (SGX_PAGE_TYPE_VA << 8),
	SGX_SECINFO_TRIM		= (SGX_PAGE_TYPE_TRIM << 8),
};

/**
 * enum sgx_attributes - the attributes field in &struct sgx_secs
 * %SGX_ATTR_INIT:		Enclave can be entered (is initialized).
 * %SGX_ATTR_DEBUG:		Allow ENCLS(EDBGRD) and ENCLS(EDBGWR).
 * %SGX_ATTR_MODE64BIT:		Tell that this a 64-bit enclave.
 * %SGX_ATTR_PROVISIONKEY:      Allow to use provisioning keys for remote
 *				attestation.
 * %SGX_ATTR_KSS:		Allow to use key separation and sharing (KSS).
 * %SGX_ATTR_EINITTOKENKEY:	Allow to use token signing key that is used to
 *				sign cryptographic tokens that can be passed to
 *				EINIT as an authorization to run an enclave.
 * %SGX_ATTR_ASYNC_EXIT_NOTIFY:	Allow enclaves to be notified after an
 *				asynchronous exit has occurred.
 */
enum sgx_attribute {
	SGX_ATTR_INIT		   = BIT(0),
	SGX_ATTR_DEBUG		   = BIT(1),
	SGX_ATTR_MODE64BIT	   = BIT(2),
				  /* BIT(3) is reserved */
	SGX_ATTR_PROVISIONKEY	   = BIT(4),
	SGX_ATTR_EINITTOKENKEY	   = BIT(5),
				  /* BIT(6) is for CET */
	SGX_ATTR_KSS		   = BIT(7),
				  /* BIT(8) is reserved */
				  /* BIT(9) is reserved */
	SGX_ATTR_ASYNC_EXIT_NOTIFY = BIT(10),
};

#endif

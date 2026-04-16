// SPDX-License-Identifier: GPL-2.0
/*  Copyright(c) 2016-20 Intel Corporation. */

#define _GNU_SOURCE
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include "baresgx/util.h"
#include "internal/sgx-defs.h"

uint64_t g_attributes = SGX_ATTR_MODE64BIT, g_misc = 0;
char *g_sgxs_path = NULL, *g_key_path = NULL, *g_sig_path = NULL;
int g_sgxs_fd = -1, g_key_fd = -1, g_sig_fd = -1;

/* ------------------------------------------------------------------ */
/* Q1 / Q2 calculation                                                */
/* ------------------------------------------------------------------ */

struct q1q2_ctx {
	BN_CTX *bn_ctx;
	BIGNUM *m, *s, *q1, *qr, *q2;
};

static void free_q1q2_ctx(struct q1q2_ctx *ctx)
{
	BN_CTX_free(ctx->bn_ctx);
	BN_free(ctx->m);
	BN_free(ctx->s);
	BN_free(ctx->q1);
	BN_free(ctx->qr);
	BN_free(ctx->q2);
}

static bool alloc_q1q2_ctx(const uint8_t *s, const uint8_t *m,
			    struct q1q2_ctx *ctx)
{
	ctx->bn_ctx = BN_CTX_new();
	ctx->s  = BN_bin2bn(s, SGX_MODULUS_SIZE, NULL);
	ctx->m  = BN_bin2bn(m, SGX_MODULUS_SIZE, NULL);
	ctx->q1 = BN_new();
	ctx->qr = BN_new();
	ctx->q2 = BN_new();

	if (!ctx->bn_ctx || !ctx->s || !ctx->m || !ctx->q1 || !ctx->qr || !ctx->q2) {
		free_q1q2_ctx(ctx);
		return false;
	}
	return true;
}

static void reverse_bytes(void *data, int len)
{
	uint8_t *p = data;
	for (int i = 0, j = len - 1; i < j; i++, j--) {
		uint8_t tmp = p[i]; p[i] = p[j]; p[j] = tmp;
	}
}

static void calc_q1q2(const uint8_t *s, const uint8_t *m,
		      uint8_t *q1, uint8_t *q2)
{
	struct q1q2_ctx ctx;
	int len;

	BARESGX_ASSERT(alloc_q1q2_ctx(s, m, &ctx));

	BARESGX_ASSERT(BN_mul(ctx.q1, ctx.s, ctx.s, ctx.bn_ctx));
	BARESGX_ASSERT(BN_div(ctx.q1, ctx.qr, ctx.q1, ctx.m, ctx.bn_ctx));
	BARESGX_ASSERT(BN_num_bytes(ctx.q1) <= SGX_MODULUS_SIZE);

	BARESGX_ASSERT(BN_mul(ctx.q2, ctx.s, ctx.qr, ctx.bn_ctx));
	BARESGX_ASSERT(BN_div(ctx.q2, NULL, ctx.q2, ctx.m, ctx.bn_ctx));
	BARESGX_ASSERT(BN_num_bytes(ctx.q2) <= SGX_MODULUS_SIZE);

	len = BN_bn2bin(ctx.q1, q1); reverse_bytes(q1, len);
	len = BN_bn2bin(ctx.q2, q2); reverse_bytes(q2, len);

	free_q1q2_ctx(&ctx);
}

/* ------------------------------------------------------------------ */
/* Signing logic                                                      */
/* ------------------------------------------------------------------ */

struct sgx_sigstruct_payload {
	struct sgx_sigstruct_header header;
	struct sgx_sigstruct_body   body;
};

static EVP_PKEY *load_key(void)
{
	BIO *bio = BIO_new_fd(g_key_fd, BIO_NOCLOSE);
	BARESGX_ASSERT(bio);

	EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
	BARESGX_ASSERT(pkey);

	BIO_free(bio);
	return pkey;
}

static void compute_mrenclave(uint8_t mrenclave[32])
{
	uint8_t buf[4096];
	ssize_t n;

	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	BARESGX_ASSERT(ctx);
	BARESGX_ASSERT(EVP_DigestInit_ex(ctx, EVP_sha256(), NULL));

	while ((n = read(g_sgxs_fd, buf, sizeof(buf))) > 0)
		BARESGX_ASSERT(EVP_DigestUpdate(ctx, buf, n));
	BARESGX_ASSERT(n == 0 && "SGXS input file read error");

	unsigned int mdlen = 32;
	BARESGX_ASSERT(EVP_DigestFinal_ex(ctx, mrenclave, &mdlen));
	EVP_MD_CTX_free(ctx);
}

static inline uint32_t build_date_yyyymmdd_hex(void)
{
    time_t t = time(NULL);
	BARESGX_ASSERT(t != ((time_t) -1));
    struct tm *tm = localtime(&t);
	BARESGX_ASSERT(tm != NULL);

    int y = tm->tm_year + 1900;
    int m = tm->tm_mon + 1;
    int d = tm->tm_mday;

    return ((y / 1000 % 10) << 28) |
           ((y / 100  % 10) << 24) |
           ((y / 10   % 10) << 20) |
           ((y        % 10) << 16) |
           ((m / 10   % 10) << 12) |
           ((m        % 10) << 8)  |
           ((d / 10   % 10) << 4)  |
           ((d        % 10));
}

static void sign_enclave(struct sgx_sigstruct *ss)
{
	uint64_t header1[2] = {0x000000E100000006, 0x0000000000010000};
	uint64_t header2[2] = {0x0000006000000101, 0x0000000100000060};
	struct sgx_sigstruct_payload payload;
	size_t siglen = SGX_MODULUS_SIZE;

	memset(ss, 0, sizeof(*ss));

	ss->header.header1[0] = header1[0];
	ss->header.header1[1] = header1[1];
	ss->header.header2[0] = header2[0];
	ss->header.header2[1] = header2[1];
	ss->exponent          = 3;
	ss->body.attributes   = g_attributes;
	ss->body.miscselect   = g_misc;
	ss->header.date       = build_date_yyyymmdd_hex();

	EVP_PKEY *pkey = load_key();

	/* Extract raw RSA modulus into the sigstruct */
	BIGNUM *n = NULL;
	EVP_PKEY_get_bn_param(pkey, "n", &n);
	BARESGX_ASSERT(n);
	BN_bn2bin(n, ss->modulus);
	BN_free(n);

	compute_mrenclave(ss->body.mrenclave);
	baresgx_info("mrenclave=%s", hex_str(ss->body.mrenclave, sizeof(ss->body.mrenclave)));

	memcpy(&payload.header, &ss->header, sizeof(ss->header));
	memcpy(&payload.body,   &ss->body,   sizeof(ss->body));

	EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
	BARESGX_ASSERT(mdctx);
	BARESGX_ASSERT(EVP_DigestSignInit(mdctx, NULL, EVP_sha256(), NULL, pkey));
	BARESGX_ASSERT(EVP_DigestSign(mdctx, ss->signature, &siglen,
				      (unsigned char *)&payload, sizeof(payload)));
	EVP_MD_CTX_free(mdctx);

	calc_q1q2(ss->signature, ss->modulus, ss->q1, ss->q2);

	/* BE -> LE */
	reverse_bytes(ss->signature, SGX_MODULUS_SIZE);
	reverse_bytes(ss->modulus,   SGX_MODULUS_SIZE);

	EVP_PKEY_free(pkey);
}

/* ------------------------------------------------------------------ */
/* main and parsing logic                                             */
/* ------------------------------------------------------------------ */

static int read_num(int i, int argc, char *argv[], uint64_t *val)
{
	errno = 0;
	if (i+1 >= argc) return -1;
	*val = strtoull(argv[++i], NULL, 0);
	if (errno) return -1;
	return 0;
}

static int parse_args(int argc, char *argv[])
{
	int positional = 0;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--attributes")) {
			BARESGX_ASSERT_RET(!read_num(i, argc, argv, &g_attributes), "attributes");
			BARESGX_ASSERT_RET(sgx_attribute_is_valid(g_attributes), "attributes");
			i++;
		} else if (!strcmp(argv[i], "--miscselect")) {
			BARESGX_ASSERT_RET(!read_num(i, argc, argv, &g_misc), "miscselect");
			BARESGX_ASSERT_RET(sgx_miscselect_is_valid(g_misc), "miscselect");
			i++;
		} else if (argv[i][0] == '-') {
			BARESGX_ASSERT_RET(0, argv[i]);
		} else {
			switch (positional++) {
			case 0:
				g_sgxs_path = argv[i];
				g_sgxs_fd = open(argv[i], O_RDONLY);
				BARESGX_ASSERT_RET(g_sgxs_fd > 0, argv[i]);
				break;
			case 1:
				g_key_path = argv[i];
				g_key_fd = open(argv[i], O_RDONLY, 0644);
				BARESGX_ASSERT_RET(g_key_fd > 0, argv[i]);
				break;
			case 2:
				g_sig_path = argv[i];
				g_sig_fd = open(argv[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
				BARESGX_ASSERT_RET(g_sig_fd > 0, argv[i]);
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
	struct sgx_sigstruct ss;

	if (parse_args(argc, argv) < 0) {
		fprintf(stderr, "Usage: %s <encl.sgxs> <key.pem> <out.sigstruct> "
			        "[--attributes <hex>] [--miscselect <hex>]\n", argv[0]);
		return -1;
	}

	baresgx_info("signing '%s' with key '%s' -> '%s'", g_sgxs_path, g_key_path, g_sig_path);
	baresgx_info("attributes=%#lx (%s) miscselect=%#lx (%s)",
		     g_attributes, sgx_attribute_to_str(g_attributes),
		     g_misc, sgx_miscselect_to_str(g_misc));

	sign_enclave(&ss);
	BARESGX_ASSERT(write(g_sig_fd, &ss, sizeof(ss)) == sizeof(ss));

	return 0;
}
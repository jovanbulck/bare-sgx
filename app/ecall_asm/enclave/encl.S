	.text
encl_entry:
	# minimal enclave: return secret
	mov     encl_secret(%rip), %rsi

	# EEXIT
	mov     %rcx, %rbx
	mov	$4, %rax
	enclu

	.data
encl_secret:
        .quad 0xdeadbeefcafebabe


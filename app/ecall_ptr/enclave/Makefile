.PHONY: all clean

CFLAGS  += -Wall -Werror -static-pie -nostdlib -ffreestanding -fPIE \
               -fno-stack-protector -mrdrnd $(INCLUDES)
LDFLAGS := -Wl,-T,test_encl.lds,--build-id=none

PRIVATE_KEY = private.pem
PUBLIC_KEY = public.pem

all: encl.sgxs encl.sig

encl.elf: test_encl_bootstrap.S test_encl.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.sgxs: %.elf
	../../../urts/elf2sgxs $^ $@
	sgxs-info summary $@ || true
	sha256sum $@ || true

%.sig: %.sgxs $(PRIVATE_KEY) 
	sgxs-sign -d $< $@ --key $(PRIVATE_KEY)
	
$(PRIVATE_KEY):
	openssl genrsa -3 3072 > $(PRIVATE_KEY)
	openssl rsa -in $(PRIVATE_KEY) -pubout -out $(PUBLIC_KEY) 

clean:
	rm -f *.elf *.o *.sgxs *.sig
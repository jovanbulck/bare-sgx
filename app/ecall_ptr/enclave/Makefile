BARESGX         = ../../..
TOOLS           = $(BARESGX)/sdk/tools

PRIVATE_KEY     = private.pem
PUBLIC_KEY      = public.pem

SOURCES         = $(shell ls *.c *.S)

CFLAGS         += -Wall -Werror -static-pie -nostdlib -ffreestanding -fPIE \
        		       -fno-stack-protector -mrdrnd $(INCLUDES)
LDFLAGS        := -Wl,-T,encl.lds,--build-id=none

all: encl.sgxs encl.sig

encl.elf: $(SOURCES)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.sgxs: %.elf
	$(TOOLS)/elf2sgxs $^ $@
	sgxs-info summary $@ || true
	sha256sum $@ || true

%.sig: %.sgxs $(PRIVATE_KEY) 
	$(TOOLS)/sgxs-sign $< $(PRIVATE_KEY) $@

$(PRIVATE_KEY):
	openssl genrsa -3 3072 > $(PRIVATE_KEY)
	openssl rsa -in $(PRIVATE_KEY) -pubout -out $(PUBLIC_KEY) 

clean:
	rm -f *.elf *.o *.sgxs *.sig
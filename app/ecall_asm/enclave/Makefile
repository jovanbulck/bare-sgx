.PHONY: all clean

CFLAGS  += -Wall -Werror -static-pie -nostdlib -ffreestanding -fPIE \
               -fno-stack-protector -mrdrnd $(INCLUDES)
LDFLAGS := -Wl,-T,encl.lds,--build-id=none

PRIVATE_KEY = private.pem
PUBLIC_KEY = public.pem

SGXS_TOOLS = ../../../bin/

all: encl.sgxs encl.sig

encl.elf: encl.S
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.sgxs: %.elf
	../../../urts/elf2sgxs $^ $@ #TODO add --nssa=2 --aexnotify here
	$(SGXS_TOOLS)sgxs-info summary $@ || true
	sha256sum $@ || true

%.sig: %.sgxs $(PRIVATE_KEY) 
	$(SGXS_TOOLS)sgxs-sign -d --attributes 0x404/0x402 $< $@ --key $(PRIVATE_KEY)
	
$(PRIVATE_KEY):
	openssl genrsa -3 3072 > $(PRIVATE_KEY)
	openssl rsa -in $(PRIVATE_KEY) -pubout -out $(PUBLIC_KEY) 

clean:
	rm -f *.elf *.o *.sgxs *.sig

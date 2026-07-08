BARESGX         = ../../..
TOOLS           = $(BARESGX)/sdk/tools
TRTS            = $(BARESGX)/sdk/trts

SUBDIRS         = $(TRTS)

PRIVATE_KEY     = private.pem
PUBLIC_KEY      = public.pem

SOURCES         = $(shell ls *.c)

INCLUDE         = -I$(BARESGX)/include/
CFLAGS         += -Wall -Werror -static-pie -nostdlib -ffreestanding -fPIE \
        		       -fno-stack-protector -mno-sse -mrdrnd -Os
LDFLAGS        := -T $(TRTS)/enclave.lds -Wl,--build-id=none -z noexecstack \
                  -lbare-trts -L$(TRTS)

BUILDDIRS       = $(SUBDIRS:%=build-%)
CLEANDIRS       = $(SUBDIRS:%=clean-%)

all: $(BUILDDIRS) encl.elf encl.sgxs encl.sig

%.elf: $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@ $(LDFLAGS)

%.sgxs: %.elf
	$(TOOLS)/elf2sgxs --nssa 2 --aexnotify $^ $@
	$(TOOLS)/sgxs-info summary $@ || true
	sha256sum $@ || true

%.sig: %.sgxs $(PRIVATE_KEY)
	$(TOOLS)/sgxs-sign $< $(PRIVATE_KEY) $@

$(PRIVATE_KEY):
	openssl genrsa -3 3072 > $(PRIVATE_KEY)
	openssl rsa -in $(PRIVATE_KEY) -pubout -out $(PUBLIC_KEY)

$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

$(CLEANDIRS):
	$(MAKE) clean -C $(@:clean-%=%)

clean: $(CLEANDIRS)
	rm -f *.elf *.o *.sgxs *.sig

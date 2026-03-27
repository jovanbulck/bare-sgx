.PHONY: all clean

CFLAGS  += -Wall -static-pie -nostdlib -ffreestanding -fPIE \
               -fno-stack-protector -mrdrnd $(INCLUDES)
LDFLAGS := -z noexecstack -Wl,-T,test_encl.lds,--build-id=none

all: encl.sgxs

encl.elf: test_encl_bootstrap.S test_encl.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

encl.sgxs: encl.elf
	../../../urts/elf2sgxs $^ $@
	sgxs-info summary $@ || true
	sha256sum $@ || true

clean:
	rm -f *.elf *.o *.sgxs

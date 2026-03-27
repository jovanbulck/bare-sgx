.PHONY: all clean

CFLAGS  += -Wall -Werror -static-pie -nostdlib -ffreestanding -fPIE \
               -fno-stack-protector -mrdrnd $(INCLUDES)
LDFLAGS := -Wl,-T,encl.lds,--build-id=none

all: encl.sgxs

encl.elf: encl.S
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

encl.sgxs: encl.elf
	../../../urts/elf2sgxs $^ $@
	sgxs-info summary $@ || true
	sha256sum $@ || true

clean:
	rm -f *.elf *.o *.sgxs

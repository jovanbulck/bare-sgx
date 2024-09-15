.PHONY: all clean

CFLAGS  += -Wall -static-pie -nostdlib -ffreestanding -fPIE \
	       -fno-stack-protector -mrdrnd $(INCLUDES)
LDFLAGS := -z noexecstack -Wl,-T,test_encl.lds,--build-id=none

encl.elf: test_encl_bootstrap.S test_encl.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.elf *.o

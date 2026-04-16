CFLAGS 	= -I../../urts/include/
SOURCES = $(shell ls *.c)
TOOLS   = $(SOURCES:.c=)

all: $(TOOLS)

sgxs-sign: sgxs-sign.c
	$(CC) $< $(CFLAGS) -lcrypto -o $@

elf2sgxs: elf2sgxs.c
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

clean:
	rm -f $(TOOLS)

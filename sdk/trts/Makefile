
BARESGX          = ../..

CC              := gcc
INCLUDE         := -I$(BARESGX)/include/
CFLAGS          += -Wall -Werror -static-pie -nostdlib -ffreestanding -fPIE \
        		  -fno-stack-protector -mno-sse -mrdrnd $(INCLUDE) -Os
AR              := ar
ARFLAGS         := -rcs

SOURCES         = $(shell ls *.c)
ASM             = $(shell ls *.S)
OBJECTS         = $(SOURCES:.c=.o) $(ASM:.S=.o)
OUTPUT          = libbare-trts.a

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(AR) $(ARFLAGS) $(OUTPUT) $(OBJECTS)

%.o : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE) -c $< -o $@

%.o : %.S
	$(AS) $(ASFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OUTPUT)

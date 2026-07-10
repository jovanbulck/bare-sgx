
BARESGX          = ../..

CC              := gcc
INCLUDE         := -I$(BARESGX)/include/
CFLAGS          += -Wall -Werror -static-pie -nostdlib -ffreestanding -fPIE \
        		  -fno-stack-protector -mno-sse -mrdrnd $(INCLUDE) -Os \
				  -DBARE_TRTS_EXCEPTION_HANDLING=1 -DBARE_TRTS_AEX_NOTIFY=1
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
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OUTPUT)

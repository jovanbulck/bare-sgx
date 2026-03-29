CC              := gcc
INCLUDE         := -Iinclude/
CFLAGS          := -Wall  -g -fPIC
AR              := ar
ARFLAGS         := -rcs

# Exclude elf2sgxs.c from the library objects
SOURCES         = $(filter-out elf2sgxs.c, $(shell ls *.c))
ASM             = $(shell ls *.S)
OBJECTS         = $(SOURCES:.c=.o) $(ASM:.S=.o)
OUTPUT          = libbaresgx-urts.a
TOOLS           = elf2sgxs

all: $(OUTPUT) $(TOOLS)

$(OUTPUT): $(OBJECTS)
	$(AR) $(ARFLAGS) $(OUTPUT) $(OBJECTS)

elf2sgxs: elf2sgxs.o
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

%.o : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE) -c $< -o $@

%.o : %.S
	$(AS) $(ASFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJECTS) elf2sgxs.o $(OUTPUT) $(TOOLS)

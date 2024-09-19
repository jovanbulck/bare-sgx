CC              := gcc
INCLUDE         := -Iinclude/
CFLAGS          := -Wall -Werror -g -fPIC
AR              := ar
ARFLAGS         := -rcs

SOURCES         = $(shell ls *.c)
ASM             = $(shell ls *.S)
OBJECTS         = $(SOURCES:.c=.o) $(ASM:.S=.o)
OUTPUT          = libbaresgx-urts.a

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(AR) $(ARFLAGS) $(OUTPUT) $(OBJECTS)

%.o : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE) -c $< -o $@

%.o : %.S
	$(AS) $(ASFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OUTPUT)

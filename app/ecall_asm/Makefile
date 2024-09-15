BARESGX         = ../../urts/
ENCLAVE         = enclave

SUBDIRS         = $(ENCLAVE) $(BARESGX)

CC              := gcc
LD              := gcc
INCLUDE         := -I$(BARESGX)/include/
CFLAGS          := -Wall -Werror -g -fPIC $(CFLAGS)
LDFLAGS         := -z noexecstack -lbaresgx-urts -L$(BARESGX) -lcrypto 

SOURCES         = $(shell ls *.c)
OBJECTS         = $(SOURCES:.c=.o)
OUTPUT          = app

BUILDDIRS       = $(SUBDIRS:%=build-%)
CLEANDIRS       = $(SUBDIRS:%=clean-%)

all: $(OUTPUT)

$(OUTPUT): $(BUILDDIRS) $(OBJECTS)
	$(LD) $(OBJECTS) $(LDFLAGS) -o $(OUTPUT) 

%.o : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

$(CLEANDIRS):
	$(MAKE) clean -C $(@:clean-%=%)

clean: $(CLEANDIRS)
	rm -f $(OBJECTS) $(OUTPUT)

BARESGX          = ..
TRTS            = $(BARESGX)/sdk/trts

CC              := gcc
INCLUDE         := -I$(BARESGX)/include/
AR              := ar
ARFLAGS         := -rcs

all: stripped

preprocess: entry_base.s entry_exception.s entry_notify.s
stripped: entry_base_stripped.s entry_exception_stripped.s entry_notify_stripped.s

cloc: entry_base.s entry_exception.s entry_notify.s
	cloc --quiet --by-file --match-f='.*\.s' $^

entry_base.s:
	$(CC) $(INCLUDE) -E -DBARE_TRTS_EXCEPTION_HANDLING=0 -DBARE_TRTS_AEX_NOTIFY=0 $(TRTS)/entry.S -o entry_base.s

entry_exception.s:
	$(CC) $(INCLUDE) -E -DBARE_TRTS_EXCEPTION_HANDLING=1 -DBARE_TRTS_AEX_NOTIFY=0 $(TRTS)/entry.S -o entry_exception.s

entry_notify.s:
	$(CC) $(INCLUDE) -E -DBARE_TRTS_EXCEPTION_HANDLING=1 -DBARE_TRTS_AEX_NOTIFY=1 $(TRTS)/entry.S -o entry_notify.s

entry_base_stripped.s: entry_base.s
	grep -Ev '#|^$$' entry_base.s > entry_base_stripped.s

entry_exception_stripped.s: entry_exception.s
	grep -Ev '#|^$$' entry_exception.s > entry_exception_stripped.s

entry_notify_stripped.s: entry_notify.s
	grep -Ev '#|^$$' entry_notify.s > entry_notify_stripped.s


clean:
	rm -f *s
# Bare-metal C enclave with with trusted runtime AEX-notify enabled exception handling + exception counting mitigation

Sample output:

```
[main.c] loaded enclave at 0x7f4683ed8000
[main.c] reading enclave memory..
	L mem at 0x7f4683ed8000 is ffffffffffffffff
[main.c] calling enclave TCS..
	L enclave returned base=0x7f4683ed0000; size=0x10000
	L enclave returned 1300 + 37 = 1337
[main.c] aep_counter after 2nd call = 5
[main.c] aep_counter reset
	L enclave returned 1300 - 37 = 1263
[main.c] aep_counter after 3th call = 96

--- Signal Caught ---
Signal Number: 4 (SIGILL)
Faulting Address: 0x55fdaec6be32
	L enclave returned CPUID result = 256

--- Test completed successfully ---
```

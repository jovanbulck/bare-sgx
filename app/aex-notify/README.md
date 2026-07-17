# Bare-metal C enclave with with trusted runtime AEX-notify enabled exception handling

Sample output:

```
[main.c] loaded enclave at 0x7f5547887000
[main.c] reading enclave memory..
	L mem at 0x7f5547887000 is ffffffffffffffff
[main.c] calling enclave TCS..
	L enclave returned base=0x7f5547880000; size=0x10000
	L enclave returned 1300 + 37 = 1337

--- Signal Caught ---
Signal Number: 4 (SIGILL)
Faulting Address: 0x5559649fad8b
	L enclave returned 1300 - 37 = 1263

--- Signal Caught ---
Signal Number: 4 (SIGILL)
Faulting Address: 0x5559649fad8b
	L enclave returned CPUID result = 256

--- Test completed successfully ---
```

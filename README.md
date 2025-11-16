# Minimal SGX Enclave Development on Bare-Metal Linux Platforms

![bare-sgx-logo](doc/bare-sgx-logo.svg)

This repository provides a minimal, fully customizable framework for developing Intel SGX enclaves directly on bare-metal Linux, without relying on bloated external SDKs. It offers a clean, low-level starting point for building minimalist enclaves in assembly or C, interfacing directly with the upstream Linux SGX driver.

By interacting directly with the SGX driver in the Linux kernel, `bare-sgx` removes the complexity and overhead of existing SGX SDKs and library OSs. The result is extremely small enclaves, often just a few pages, tailored to a specific purpose and excluding _all_ other unnecessary code and features. Therefore, `bare-sgx` provides a truly minimal trusted computing base while avoiding fragile dependencies that could hinder portability or long-term reproducibility.

**License.** `bare-sgx` is free software, licensed under [GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0). The initial code was forked from the [selftests/sgx](https://github.com/torvalds/linux/tree/master/tools/testing/selftests/sgx) test enclave in the Linux kernel repository, following a [discussion](https://lore.kernel.org/all/da0cfb1e-e347-f7f2-ac72-aec0ee0d867d@intel.com/) on the kernel mailing list. 

## :question: Use Cases

`bare-sgx` aims to enable innovative SGX enclave research, both offensive and defensive, and in improving the long-term reproducibility of research artifacts.

Example use cases include:

- **Developing minimal-trust enclaves** with complete control over the trusted computing base, e.g., to support formal verification efforts.
- **Packaging research artifacts or proof-of-concepts** without fragile and bloated SDK dependencies, relying solely on the stable Linux kernel interface to ensure long-term reproducibility and portability.
- **Rapid prototyping and experimentation** with new attacks, defenses, and kernel extensions.

## :sparkles: Features

### Untrusted runtime

- Minimal pure-C **[untrusted runtime](urts/)** for building, signing, and loading minimal enclave images (packaged in a custom ELF format). The only dependencies are OpenSSL and the upstream `/dev/sgx_enclave` kernel driver.

- **[SGX-Step](https://github.com/jovanbulck/sgx-step) integration** for rapid attack prototyping, including single-stepping and controlled-channel attacks.

### Trusted runtime

- Minimal and **fully customizable enclave skeletons** in pure [assembly](app/ecall_asm/enclave) with optional bootstrapping to [C code](app/ecall_ptr/enclave).

- (Optional) **[`bare-trts`](https://github.com/KobeSauwens/bare-sgx-thesis/tree/main/trts/bare-trts) trusted runtime** (to be upstreamed), featuring:
    - Auto-generated secure bridge code for interfaces defined in Enclave Definition Language (EDL) via Intel's unmodified `edger8r` tool.
    - Minimal embedded `malloc` implementation from FreeRTOS.

- [HACL* integration](https://github.com/KobeSauwens/bare-sgx-thesis/tree/main/app/bare-crypto-app/enclave) for formally verified **cryptographic primitives** (to be upstreamed)

## :checkered_flag: Wishlist / Roadmap

- More and improved example enclaves.
- Support [SGXS format](https://github.com/fortanix/rust-sgx/tree/master/intel-sgx/sgxs-tools) for building and loading enclaves, replacing the custom ELF format currently used by the URTS loader. (Note: SGXS files can currently be extracted using [sgx-tracer](https://github.com/pandora-tee/sgx-tracer).)
- Exception handling and AEX-Notify support (work in progress)

# Minimal SGX Enclave Development on Bare-Metal Linux Platforms [![CI](https://github.com/jovanbulck/bare-sgx/actions/workflows/ci.yaml/badge.svg)](https://github.com/jovanbulck/bare-sgx/actions/workflows/ci.yaml) [![License](https://img.shields.io/badge/License-GPLv2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0)

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="doc/bare-sgx-logo-white.svg" align="left">
  <source media="(prefers-color-scheme: light)" srcset="doc/bare-sgx-logo.svg" align="left">
  <img alt="bare-sgx-logo" src="doc/bare-sgx-logo.svg" align="left">
</picture>

This repository provides a minimal, fully customizable framework for developing Intel SGX enclaves directly on bare-metal Linux, without relying on bloated external SDKs. It offers a clean, low-level starting point for building minimalist enclaves in assembly or C, interfacing directly with the upstream Linux SGX driver.

By interacting directly with the SGX driver in the Linux kernel, `bare-sgx` removes the complexity and overhead of existing SGX SDKs and library OSs. The result is extremely small enclaves, often just a few pages, tailored to a specific purpose and excluding _all_ other unnecessary code and features. Therefore, `bare-sgx` provides a truly minimal trusted computing base while avoiding fragile dependencies that could hinder portability or long-term reproducibility.

**License.** `bare-sgx` is free software, licensed under [GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0). The initial code was forked from the [selftests/sgx](https://github.com/torvalds/linux/tree/master/tools/testing/selftests/sgx) test enclave in the Linux kernel repository, following a [discussion](https://lore.kernel.org/all/da0cfb1e-e347-f7f2-ac72-aec0ee0d867d@intel.com/) on the kernel mailing list. 

## :question: Use Cases

`bare-sgx` aims to enable innovative SGX enclave research, both offensive and defensive, and to improve the long-term reproducibility of research artifacts.

Example use cases include:

- **Developing minimal-trust enclaves** with complete control over the trusted computing base, e.g., to support formal verification efforts.
- **Packaging research artifacts or proof-of-concepts** without fragile and bloated SDK dependencies, relying solely on the stable Linux kernel interface to ensure long-term reproducibility and portability.
- **Rapid prototyping and experimentation** with new attacks, defenses, and kernel extensions.
- Minimal, self-contained **enclave test framework** for CI/CD integration (e.g., as used in the [Pandora-SGX](https://github.com/pandora-tee/pandora-examples) symbolic execution tool).

## :sparkles: Features

### Untrusted Runtime

- Minimal pure-C **[untrusted runtime](urts/)** for loading minimal enclave images (packaged in the standardized [SGXS format](https://github.com/fortanix/rust-sgx/tree/master/intel-sgx/sgxs-tools)). The only dependency is the upstream `/dev/sgx_enclave` Linux kernel driver.
- **[SGX-Step](https://github.com/jovanbulck/sgx-step) integration** for rapid attack prototyping, including single-stepping and controlled-channel attacks.
- **[Buildroot](buildroot)** integration for packaging `bare-sgx` enclaves in minimal, self-contained VM images with virtualized SGX support.
- [**CI/CD**](https://github.com/jovanbulck/bare-sgx/actions/) pipeline for automated building and testing.

### Mini SDK and Trusted Runtime

- Minimal **SDK tools** for [building](sdk/tools/elf2sgxs.c) and [signing](sdk/tools/sgxs-sign.c) bare-metal enclave images in pure C or assembly. The only dependencies are OpenSSL and standard build tools (make, gcc).
- Minimal and **fully customizable enclave skeletons** in pure [assembly](app/ecall_asm/enclave) with optional bootstrapping to [C code](app/ecall_ptr/enclave).
- Optional **[`bare-trts`](https://github.com/KobeSauwens/bare-sgx-thesis/tree/main/trts/bare-trts) trusted runtime**, featuring:
    - Minimal ABI sanitization [entry stub](sdk/trts/entry.S)
    - Minimal [C library](sdk/trts/trts.c) with secure pointer validation functions.
    - (to be [upstreamed](https://github.com/KobeSauwens/bare-sgx-thesis/tree/main/trts/bare-trts)): Auto-generated secure bridge code for interfaces defined in Enclave Definition Language (EDL) via Intel's unmodified `edger8r` tool.
    - (to be [upstreamed](https://github.com/KobeSauwens/bare-sgx-thesis/tree/main/trts/bare-trts)): Minimal embedded `malloc` (current implementation from FreeRTOS).
- (to be [upstreamed](https://github.com/KobeSauwens/bare-sgx-thesis/tree/main/app/bare-crypto-app/enclave)): Optional HACL* integration for formally verified **cryptographic primitives**

## :checkered_flag: Wishlist / Roadmap

- [x] ~Support [SGXS format](https://github.com/fortanix/rust-sgx/tree/master/intel-sgx/sgxs-tools) for building and loading enclaves, replacing the custom ELF format currently used by the URTS loader.~
- [x] ~Minimal trusted runtime with ABI/API sanitization~
- [ ] More and improved example enclaves.
- [ ] Exception handling and AEX-Notify support (work in progress).
- [ ] Formal verification.

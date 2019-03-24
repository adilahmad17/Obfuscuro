# OBFUSCURO

This is the repository for Obfuscuro, a commodity obfuscation
Engine for Intel SGX, accepted at NDSS 2019.

[Paper](https://web.ics.purdue.edu/~ahmad37/papers/obfuscuro.pdf)

# Installation Procedure

## Tested System Specs

This repository has been tested to work with the following systems:

- Ubuntu 16.04.5 (Linux kernel version 4.15.0-42-generic)
  CPU: Intel Core-i7 8600 (3.2 Ghz) with 16 GB RAM (128MB for EPC)

## STEP1: LLVM Compiler

To install the LLVM compiler, please do as follows:

`cd scripts/`
`./build-llvm.sh`

## STEP2: Intel SGX Driver

To install the Intel SGX Driver for Linux, please do as follows:

`cd scripts`
`./install-driver.sh`

## STEP3: SGX SDK

To install the intel SGX SDK for linux, please do as follows:

`cd libs/linux-sgx`
`./install.sh`

## STEP4: Run an example

To test one of the provided examples, please do as follows:

`cd eval/sum`
`./run-after.sh`

# Contact

Adil Ahmad - adilahmad17(at)gmail.com
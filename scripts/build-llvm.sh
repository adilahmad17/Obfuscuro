#!/bin/bash -e

mkdir -p ../llvm/build
pushd ../llvm/build

pushd ../llvm-4.0.0.src/projects/compiler-rt/lib/obfuscuro
  make clean && make
popd


cmake -DLLVM_TARGETS_TO_BUILD=X86 \
      -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_ENABLE_ASSERTIONS=On \
      ../llvm-4.0.0.src
make clean
make -j`nproc`
popd

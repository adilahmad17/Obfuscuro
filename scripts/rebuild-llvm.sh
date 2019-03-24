#!/bin/bash -e

pushd ../llvm/build

pushd ../llvm-4.0.0.src/projects/compiler-rt/lib/obfuscuro
  make clean && make
popd

make -j`nproc`

popd

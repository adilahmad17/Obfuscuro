#!/bin/bash

pushd ../../../../../../
./build-llvm.sh
popd

make clean
make

env RERAND_OPTIONS=verbosity=1 ./build/code-reloc.after-code

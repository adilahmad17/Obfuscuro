#!/bin/bash

make clean && make

pushd ../../../../../../scripts
	./rebuild-llvm.sh
if [ "$1" == "r" ]; then
	./test-llvm.sh
fi

popd

#!/bin/bash -e

pushd ../

git clone https://github.com/intel/linux-sgx-driver.git

pushd linux-sgx-driver
	git checkout b73806087e14bf16eceb6f96f2689a459aa56144
	make clean && make
	sudo mkdir -p "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"
	sudo cp isgx.ko "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"
	sudo sh -c "cat /etc/modules | grep -Fxq isgx || echo isgx >> /etc/modules"
	sudo /sbin/depmod
	sudo /sbin/modprobe isgx
popd

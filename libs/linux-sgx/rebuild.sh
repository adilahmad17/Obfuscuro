#!/bin/bash -e

CURDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd $CURDIR

make sdk_install_pkg

echo "yes" | linux/installer/bin/sgx_linux_x64_sdk_1.8.100.37689.bin

if [ -d "/opt/intel/sgxpsw" ]; then
    pushd /opt/intel/sgxpsw
    sudo ./uninstall.sh
    popd
fi

pushd psw
	make
popd


make psw_install_pkg

sudo linux/installer/bin/sgx_linux_x64_psw_1.8.100.37689.bin

pushd psw/urts/linux/
	cp libsgx_urts.so ../../../sgxsdk/lib64/
popd

popd # CURDIR

echo "[*] Rebuild was successful"

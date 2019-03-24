#!/bin/bash -e

source ../../libs/linux-sgx/sgxsdk/environment

REASLR=../../ make clean
REASLR=../../ make

sudo ./app

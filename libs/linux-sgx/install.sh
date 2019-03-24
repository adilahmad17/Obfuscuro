#!/bin/bash -e

sudo apt-get install build-essential ocaml automake autoconf libtool wget python

sudo apt-get install libssl-dev libcurl4-openssl-dev protobuf-compiler libprotobuf-dev

./download_prebuilt.sh

make clean

make

make sdk_install_pkg 

./rebuild.sh

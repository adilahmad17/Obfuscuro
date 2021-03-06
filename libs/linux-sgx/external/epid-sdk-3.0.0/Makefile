#!/usr/bin/make -f
include ../../buildenv.mk

#variables are set by configure
prefix = /usr/local
installdir = $(prefix)/
epidinstalldir = $(installdir)/epid-sdk
CFLAGS = 

#g++ flags
CXX_FLAGS := $(CFLAGS) -O2 -Werror -Wall -Wextra -Wno-missing-braces \
	-Wno-missing-field-initializers -Wno-unknown-pragmas -Wno-unused-function \
	-fno-strict-aliasing -Wno-unused-but-set-variable -Wno-comment -DNDEBUG -fPIC
#gcc flags
CC_FLAGS := $(CXX_FLAGS)  -D_IPP_v50_ -D_IPP_DEBUG

#intel c++ compiler flags
ICPC_FLAGS := $(CFLAGS) -O2 -Werror -Wall -Wextra -DNDEBUG
#intel c compiler flags
ICC_FLAGS := $(ICPC_FLAGS) -D_IPP_v50_ -D_IPP_DEBUG

#check if architecture was specified
#or take it by shell command
ifeq ($(findstring -m32,$(CFLAGS)),-m32)
	ARCH = x86
	CXXFLAGS += -m32
else ifeq ($(findstring -m64,$(CFLAGS)),-m64)
	ARCH = x86_64
	CXXFLAGS += -m64
else
	ARCH := $(shell uname -m)
endif

#set tools and flags depending on specified compiler
ifeq ($(CC),icc)
	CFLAGS := $(ICC_FLAGS)
	CXXFLAGS := $(ICPC_FLAGS)
	AR = xiar
else ifeq ($(CC),cc)
	CFLAGS := $(CC_FLAGS)
ifeq ($(ARCH),x86)
	CFLAGS += -m32
endif
else
	CFLAGS := $(CC_FLAGS)
	CXXFLAGS := $(CXX_FLAGS)
	ARCH = x86
endif

#set c++11 for all c++ compilers
CXXFLAGS += -std=c++0x

#set flags for unit tests executables
GTEST_FLAGS = --gtest_color=yes \
	--gtest_print_time=1 \
	--gtest_output=xml

export CC AR LDFLAGS ARCH CFLAGS
export CXX CXXFLAGS
export epidinstalldir
export GTEST_FLAGS

#just output
$(info $$ccompiler is [${CC}])
$(info $$cxxcompiler is [${CXX}])
$(info $$architecture is [${ARCH}])

#targets part
all: ipp common member verifier dropt 

clean:
	$(MAKE) -C ./ext/ipp/sources/ippcp clean
	$(MAKE) -C ./ext/ipp/sources/ippcpepid clean
	$(MAKE) -C ./epid/common/ clean
	$(MAKE) -C ./epid/member/ clean
	$(MAKE) -C ./epid/verifier/ clean
	$(MAKE) -C ./example/util/ clean
	$(MAKE) -C ./example/ clean
	$(MAKE) -C ./tools/ clean
	$(MAKE) -C ./ext/gtest/ clean
	$(MAKE) -C ./epid/common-testhelper/ clean
	$(MAKE) -C ./ext/dropt/ clean

install:
	$(MAKE) -C ./ext/ipp/sources/ippcp install
	$(MAKE) -C ./ext/ipp/sources/ippcpepid install
	$(MAKE) -C ./epid/common/ install
	$(MAKE) -C ./epid/member/ install
	$(MAKE) -C ./epid/verifier/ install
	$(MAKE) -C ./example/ install
	$(MAKE) -C ./tools/ install
	$(MAKE) -C ./epid/common-testhelper/ install

uninstall:
	rm -rf  $(epidinstalldir)

ipp:
	$(MAKE) -C ./ext/ipp/sources/ippcp all
	$(MAKE) -C ./ext/ipp/sources/ippcpepid all ARCH=$(ARCH)

common:
	$(MAKE) -C ./epid/common/ all

member:
	$(MAKE) -C ./epid/member/ all

verifier:
	$(MAKE) -C ./epid/verifier/ all

dropt:
	$(MAKE) -C ./ext/dropt/ all

examples_util:
	$(MAKE) -C ./example/util/ all

check:
	$(MAKE) -C ./ext/gtest/ all
	$(MAKE) -C ./epid/common-testhelper/ all
	$(MAKE) -C ./epid/common-testhelper/ check
	$(MAKE) -C ./epid/common/ check
	$(MAKE) -C ./epid/member/ check
	$(MAKE) -C ./epid/verifier/ check

examples:
	$(MAKE) -C ./example/ all

epid_tools:
	$(MAKE) -C ./tools/ all

build: all check install

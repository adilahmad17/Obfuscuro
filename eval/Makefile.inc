# Program Specifications #
COMMON := ../common
UNTRUSTED := ../common/untrusted
TRUSTED := ../common/trusted

######## SGX SDK Settings ########

SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_DEBUG ?= 1


ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ORIGINAL_CC = clang
ORIGINAL_CXX = clang++
ifdef REASLR
        CC = ${REASLR}/llvm/build/bin/clang
        CXX = ${REASLR}/llvm/build/bin/clang++
        REASLR_RT_DIR = ${REASLR}/llvm/llvm-4.0.0.src/projects/compiler-rt/lib/obfuscuro
        REASLR_RT = ${REASLR_RT_DIR}/build/libsgx_obfuscuro.a
        REASLR_RT_CC = gcc
        REASLR_RT_CXX = g++
        REASLR_CFLAGS = -mcmodel=large -DSGX -DRERAND
else
        CC = clang
        CXX = clang++
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
        SGX_COMMON_CFLAGS += -O0 -g
else
        SGX_COMMON_CFLAGS += -O2
endif

######## App Settings ########

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

App_Cpp_Files := $(wildcard ${UNTRUSTED}/*.cpp)
App_Include_Paths := -I${COMMON}/Include -IApp -I$(SGX_SDK)/include
App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(App_Include_Paths)

# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
        App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
        App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
        App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

App_Cpp_Flags := $(App_C_Flags) -std=c++11
App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lpthread 

ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif

App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)

App_Name := app

######## Enclave Settings ########

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif
Crypto_Library_Name := sgx_tcrypto

Enclave_Cpp_Files := Enclave.cpp $(wildcard ${TRUSTED}/*.cpp) ${TRUSTED}/.reflect.real.cpp
Enclave_Include_Paths := -I${COMMON}/Include -I${TRUSTED} -I$(SGX_SDK)/include \
       -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport \
      -I$(REASLR)/llvm/llvm-4.0.0.src/projects/compiler-rt/include

Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) -std=c++03 -nostdinc++

# To generate a proper enclave, it is recommended to follow below guideline to link the trusted libraries:
#    1. Link sgx_trts with the `--whole-archive' and `--no-whole-archive' options,
#       so that the whole content of trts is included in the enclave.
#    2. For other libraries, you just need to pull the required symbols.
#       Use `--start-group' and `--end-group' to link these libraries.
# Do NOT move the libraries linked with `--start-group' and `--end-group' within `--whole-archive' and `--no-whole-archive' options.
# Otherwise, you may get some undesirable errors.
Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tstdcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=${TRUSTED}/Enclave.lds

Enclave_Cpp_Objects := $(Enclave_Cpp_Files:.cpp=.o)
Enclave_Name_Without_Symbol := enclave.without.symbol.so
Enclave_Name_With_Symbol := enclave.with.symbol.so
Enclave_Name := enclave.so
Signed_Enclave_Name := enclave.signed.so
Enclave_Config_File := ${TRUSTED}/Enclave.config.xml

ifeq ($(SGX_MODE), HW)
ifeq ($(SGX_DEBUG), 1)
	Build_Mode = HW_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	Build_Mode = HW_PRERELEASE
else
	Build_Mode = HW_RELEASE
endif
else
ifeq ($(SGX_DEBUG), 1)
	Build_Mode = SIM_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	Build_Mode = SIM_PRERELEASE
else
	Build_Mode = SIM_RELEASE
endif
endif


.PHONY: all run

ifeq ($(Build_Mode), HW_RELEASE)
all: .config_$(Build_Mode)_$(SGX_ARCH) $(App_Name) $(Enclave_Name)
	@echo "The project has been built in release hardware mode."
	@echo "Please sign the $(Enclave_Name) first with your signing key before you run the $(App_Name) to launch and access the enclave."
	@echo "To sign the enclave use the command:"
	@echo "   $(SGX_ENCLAVE_SIGNER) sign -key <your key> -enclave $(Enclave_Name) -out <$(Signed_Enclave_Name)> -config $(Enclave_Config_File)"
	@echo "You can also sign the enclave using an external signing tool."
	@echo "To build the project in simulation mode set SGX_MODE=SIM. To build the project in prerelease mode set SGX_PRERELEASE=1 and SGX_MODE=HW."
else
all: .config_$(Build_Mode)_$(SGX_ARCH) $(App_Name) $(Signed_Enclave_Name)
ifeq ($(Build_Mode), HW_DEBUG)
	@echo "The project has been built in debug hardware mode."
else ifeq ($(Build_Mode), SIM_DEBUG)
	@echo "The project has been built in debug simulation mode."
else ifeq ($(Build_Mode), HW_PRERELEASE)
	@echo "The project has been built in pre-release hardware mode."
else ifeq ($(Build_Mode), SIM_PRERELEASE)
	@echo "The project has been built in pre-release simulation mode."
else
	@echo "The project has been built in release simulation mode."
endif
endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/$(App_Name)
	@echo "RUN  =>  $(App_Name) [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif

######## App Objects ########

${UNTRUSTED}/Enclave_u.c: $(SGX_EDGER8R) ${TRUSTED}/Enclave.edl
	@cd ${UNTRUSTED} && $(SGX_EDGER8R) --untrusted Enclave.edl --search-path ../trusted --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

${UNTRUSTED}/Enclave_u.o: ${UNTRUSTED}/Enclave_u.c
	@$(CC) $(App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

${UNTRUSTED}/%.o: ${UNTRUSTED}/%.cpp
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(App_Name): ${UNTRUSTED}/Enclave_u.o $(App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(App_Link_Flags)
	@echo "LINK =>  $@"

.config_$(Build_Mode)_$(SGX_ARCH):
	@rm -f .config_* $(App_Name) $(Enclave_Name) $(Signed_Enclave_Name) $(App_Cpp_Objects) ${UNTRUSTED}/Enclave_u.* $(Enclave_Cpp_Objects) Enclave/Enclave_t.*
	@touch .config_$(Build_Mode)_$(SGX_ARCH)

######## Enclave Objects ########

${TRUSTED}/Enclave_t.c: $(SGX_EDGER8R) ${TRUSTED}/Enclave.edl
	@cd ${TRUSTED} && $(SGX_EDGER8R) --trusted Enclave.edl --search-path ${TRUSTED} --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

${TRUSTED}/Enclave_t.o: ${TRUSTED}/Enclave_t.c
	@$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

$(Enclave_Name_Without_Symbol): ${TRUSTED}/Enclave_t.o $(Enclave_Cpp_Objects) $(Enclave_C_Objects) $(REASLR_RT)
	$(CXX) $^ -o $@ $(Enclave_Link_Flags) -fPIC
	@echo "LINK =>  $@"
	./${TRUSTED}/extract_addr.sh $(Enclave_Name_Without_Symbol) > ${TRUSTED}/.reflect.real.cpp

$(Enclave_Name_With_Symbol) : ${TRUSTED}/Enclave_t.o $(Enclave_Cpp_Objects) $(Enclave_C_Objects) $(REASLR_RT)
	$(CXX) ${TRUSTED}/.reflect.real.cpp $^ -o $@ $(Enclave_Link_Flags) -fPIC
	@echo "LINK =>  $@"
	./${TRUSTED}/extract_addr.sh $(Enclave_Name_With_Symbol) > ${TRUSTED}/.reflect.real.cpp

%.o: %.cpp
	@$(CXX) $(Enclave_Cpp_Flags) $(REASLR_CFLAGS) -c $< -o $@
	@echo "CXX  <=  $<"

${TRUSTED}/.reflect.real.o: $(Enclave_Name_Without_Symbol) $(Enclave_Name_With_Symbol)
	@echo "Built .reflect.real.cpp"
	gcc -c ${TRUSTED}/.reflect.real.cpp -o $@

${TRUSTED}/%.o: ${TRUSTED}/%.cpp
	@$(CXX) $(Enclave_Cpp_Flags) $(REASLR_CFLAGS) -c $< -o $@
	@echo "CXX  <=  $<"

$(REASLR_RT):
	echo "abc"
	(cd ${REASLR_RT_DIR} && make clean && SGX_PF_DIR=${ROOT} make CC=${REASLR_RT_CC} CXX=${REASLR_RT_CXX} )
	@echo "Built ${REASLR_RT}"

$(Enclave_Name): ${TRUSTED}/Enclave_t.o $(Enclave_Cpp_Objects) $(Enclave_C_Objects) $(REASLR_RT)
	$(CXX) $^ -o $@ $(Enclave_Link_Flags) -fPIC
	@echo "LINK =>  $@"

$(Signed_Enclave_Name): $(Enclave_Name)
	@$(SGX_ENCLAVE_SIGNER) sign -key ${TRUSTED}/Enclave_private.pem -enclave $(Enclave_Name) -out $@ -config $(Enclave_Config_File) -ignore-rel-error 
	@echo "SIGN =>  $@"

.PHONY: clean

clean:
	@rm -f .config_* $(App_Name) $(Enclave_Name) $(Signed_Enclave_Name) $(App_Cpp_Objects) ${UNTRUSTED}/Enclave_u.* $(Enclave_Cpp_Objects) ${TRUSTED}/Enclave_t.* *.so ${TRUSTED}/*.o ${UNTRUSTED}/*.o

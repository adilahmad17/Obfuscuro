
BUILD_DIR=build
SRC_DIR=TestCases

SRCS := $(wildcard $(SRC_DIR)/*.cc)
BEFORE := $(patsubst $(SRC_DIR)/%.cc,$(BUILD_DIR)/%.before,$(SRCS))
AFTER := $(patsubst $(SRC_DIR)/%.cc,$(BUILD_DIR)/%.after,$(SRCS))
AFTER_CODE := $(patsubst $(SRC_DIR)/%.cc,$(BUILD_DIR)/%.after-code,$(SRCS))
AFTER_DATA := $(patsubst $(SRC_DIR)/%.cc,$(BUILD_DIR)/%.after-data,$(SRCS))

CLANG=${CURDIR}/../../../../../build/bin/clang++
DIS=${CURDIR}/../../../../../build/bin/llvm-dis

CFLAGS=-g -O0 --static
LFLAGS=-Wl,-emit-relocs

# Debug flags.
ifeq ($V,1)
	COMPILE_DEBUG=-mllvm -rerand-debug=1 -mllvm -rerand-code-debug=1 -mllvm -stats
	RUNTIME_DEBUG=RERAND_OPTIONS='verbosity=1'
endif

RERAND_COMMON_FLAGS=-fsanitize=rerand ${CFLAGS} ${COMPILE_DEBUG}
RERAND_DATA_FLAGS=${RERAND_COMMON_FLAGS} -mllvm -rerand-inst-data=1 -mllvm -rerand-inst-code=0 -D__RERAND_DATA__
RERAND_CODE_FLAGS=${RERAND_COMMON_FLAGS} -mllvm -rerand-inst-data=0 -mllvm -rerand-inst-code=1 -D__RERAND_CODE__
RERAND_FLAGS=${RERAND_COMMON_FLAGS} -mllvm -rerand-inst-data=1 -mllvm -rerand-inst-code=1  -D__RERAND_DATA__ -D__RERAND_CODE__

all: $(BEFORE) $(AFTER) $(AFTER_CODE) $(AFTER_DATA)

$(BUILD_DIR)/%.before: $(SRC_DIR)/%.cc
	${CLANG} ${CFLAGS} ${LFLAGS} $< -o $@

$(BUILD_DIR)/%.after: $(SRC_DIR)/%.cc
	${CLANG} ${RERAND_FLAGS} ${LFLAGS} $< -o $@

$(BUILD_DIR)/%.after-code: $(SRC_DIR)/%.cc
	${CLANG} ${RERAND_CODE_FLAGS} ${LFLAGS} $< -o $@

$(BUILD_DIR)/%.after-data: $(SRC_DIR)/%.cc
	${CLANG} ${RERAND_DATA_FLAGS} ${LFLAGS} $< -o $@

clean:
	rm -f $(BEFORE) $(AFTER) $(AFTER_CODE) $(AFTER_DATA)



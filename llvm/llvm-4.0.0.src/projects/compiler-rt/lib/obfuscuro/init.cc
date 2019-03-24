#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef SGX
#else
#include <sys/mman.h>
#endif

#ifndef SGX
#include "sanitizer_common/sanitizer_procmaps.h"
#include "sanitizer_common/sanitizer_flags.h"
#include "sanitizer_common/sanitizer_flag_parser.h"
#endif

#include "common.h"
#include "oram.h"

int enable_debug_out = false;

scratchbuf_t scratch;
scratchbuf_t scratch_data;

extern "C" void __init_sgx() {
  scratch.buf = (char*) get_codepad_addr();
  scratch_data.buf = (char*) get_datapad_addr();
  if (scratch.buf == NULL || scratch_data.buf == NULL) CHECK(false);
  return;
}

class Initializer {
public:
  Initializer() {
    __init_sgx();
  }
};

static Initializer initializer;


#ifndef __COMMON_H__
#define __COMMON_H__

#include "../../include/sanitizer/obfuscuro_interface.h"

typedef unsigned long ADDRTY;
typedef unsigned long SIZETY;

extern bool rerand_init_is_running;
extern int enable_debug_out;
extern int rerand_oram;

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define NOINLINE
#define INTERFACE_ATTRIBUTE

#define nullptr 0
#define internal_memcpy memcpy

extern "C" unsigned long get_codepad_addr(void);
extern "C" unsigned long get_datapad_addr(void);

extern "C" void eprintf(const char *fmt, ...);
extern "C" void sgx_print_hex(unsigned long d);
extern "C" void sgx_print_string(const char *string);
extern "C" void sgx_print_integer(unsigned long int d);

#define CHECK(pred)                                     \
  do {                                                  \
    if (!(pred)) {                                      \
      eprintf("[RND][ERR] CHECK FAIL (" #pred ")\n");    \
      eprintf("\t@(%s:%d)\n", __FILE__, __LINE__);       \
      assert(false);                                    \
    }                                                   \
  } while (0);

#define DOUT(...)
#define SGXOUT(...) do {                                \
    if (enable_debug_out) {                             \
      eprintf("[RND] (%s:%d) ", __FUNCTION__, __LINE__); \
      eprintf(__VA_ARGS__);                              \
    }                                                   \
  } while (0)
#define FATAL(...) do {                                 \
    eprintf("[FATAL] (%s:%d) ", __FUNCTION__, __LINE__); \
    eprintf(__VA_ARGS__);                                \
    assert( false );                                    \
  } while (0)

#endif // __RERAND_COMMON_H__

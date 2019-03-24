#include <stdlib.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

void sgx_print_string(const char *);
void sgx_print_integer(unsigned long d);
void sgx_print_hex(unsigned long d);
void* sgx_read_executable(size_t* size);

unsigned long get_codepad_addr(void);
unsigned long get_datapad_addr(void);

#if defined(__cplusplus)
}
#endif

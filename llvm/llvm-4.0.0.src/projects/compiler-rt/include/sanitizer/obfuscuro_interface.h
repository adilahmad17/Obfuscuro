#ifndef SANITIZER_OBFUSCURO_INTERFACE_H
#define SANITIZER_OBFUSCURO_INTERFACE_H

#ifndef SGX
#include <sanitizer/common_interface_defs.h>
#endif

typedef enum {
  __CODE,
  __DATA
} OTYPE;

typedef unsigned long ADDRTY;

#ifdef __cplusplus
extern "C" {
#endif
  // This function is to populate the code and data of the program
  // into the ORAM tree.
  void __obfuscuro_populate(unsigned long, unsigned long, OTYPE);

  // This function is to fetch the result of an "oblivious" memory
  // object to another "non-oblivious" object with a particular size.
  void __obfuscuro_fetch(unsigned long, unsigned long, unsigned long);

  // For debugging purposes
  ADDRTY __rerand_oram_tree_addr_start(void);
  ADDRTY __rerand_oram_tree_addr_end(void);
  ADDRTY __rerand_oram_scratch_addr(int type);
  unsigned int __rerand_get_executed_code_blocks(); 
  unsigned int __rerand_get_fetched_data_blocks(); 

#ifdef SGX
  void __rerand_init_sgx(void);
#endif

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SANITIZER_RERAND_INTERFACE_H

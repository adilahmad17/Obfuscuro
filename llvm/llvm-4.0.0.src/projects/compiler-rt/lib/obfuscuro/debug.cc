#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#include "common.h"
#include "oram.h"

#ifndef SGX
#include "interception/interception.h"
#include "sanitizer_common/sanitizer_common.h"
#endif //SGX

// Adil. Used for debugging purposes
extern "C"
NOINLINE INTERFACE_ATTRIBUTE
ADDRTY __rerand_oram_tree_addr_start(void)
{
  return (ADDRTY) &otree_code;
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
ADDRTY __rerand_oram_tree_addr_end(void)
{
  return (ADDRTY) &otree_code[NUM_TREE_CODE_NODES];
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
ADDRTY __rerand_oram_scratch_addr(int type)
{
  if (type == 0) {
    return (ADDRTY) &scratch;
  } else {
    return (ADDRTY) &scratch_data;
  }
}

extern "C" void e_time_start(void);
extern "C" void e_time_end(void);

//byunggill for debugging
extern "C"
NOINLINE
void __rerand_print_registers(void)
{
  register unsigned long int rax asm ("rax");
  register unsigned long int rbx asm ("rbx");
  int rax_stack = rax;
  int rbx_stack = rbx;
}
// Debugging functions end.

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

#define __PUSH_CONTEXT_ASM " push %rax\n\t" \
                           " push %rbx\n\t" \
                           " push %rcx\n\t" \
                           " push %rdx\n\t" \
                           " push %rsi\n\t" \
                           " push %rdi\n\t" \
                           " push %r8\n\t"  \
                           " push %r9\n\t"  \
                           " push %r10\n\t" \
                           " push %r11\n\t" \
                           " push %r12\n\t" \
                           " push %r13\n\t" \
                           " pushfq\n\t"    \

#define __POP_CONTEXT_ASM " popfq\n\t"    \
                          " pop %r13\n\t" \
                          " pop %r12\n\t" \
                          " pop %r11\n\t" \
                          " pop %r10\n\t" \
                          " pop %r9\n\t"  \
                          " pop %r8\n\t"  \
                          " pop %rdi\n\t" \
                          " pop %rsi\n\t" \
                          " pop %rdx\n\t" \
                          " pop %rcx\n\t" \
                          " pop %rbx\n\t" \
                          " pop %rax\n\t"

static int stack_population = 0;

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __obfuscuro_fetch(ADDRTY addr1, ADDRTY addr2, SIZETY size) {
  ADDRTY new_addr = (ADDRTY) otranslate(addr2, __DATA);
  internal_memcpy((void*) addr1, (void*) new_addr, size);
  return;
}

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
void __obfuscuro_populate(ADDRTY addr1, SIZETY size, OTYPE type) {
  populate_oram(addr1, size, (OTYPE) type);
  return;
}

__asm__(
".global __obfuscuro_code_loop_handler\n\t"
"__obfuscuro_code_loop_handler:\n\t"
__PUSH_CONTEXT_ASM
"call __obfuscuro_code_loop_handler_inner\n\t"
"movq %rax, %r15\n\t"
__POP_CONTEXT_ASM
"jmp %r15"
);

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
ADDRTY __obfuscuro_code_loop_handler(void);

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
ADDRTY __obfuscuro_code_loop_handler_inner(void) {
  register ADDRTY dst_addr asm ("r15");
  ADDRTY new_addr = (ADDRTY) otranslate(dst_addr, __CODE);
  return new_addr;
}

// Byunggill
__asm__(
".global __obfuscuro_data_addr_translate\n\t"
"__obfuscuro_data_addr_translate:\n\t"
__PUSH_CONTEXT_ASM
"call __obfuscuro_data_addr_translate_inner\n\t"
"mov %rax, %r14\n\t" //TODO. Exception! it write the result to R14 instead of R15, todo is just for highlighting
__POP_CONTEXT_ASM
"ret" //not jump this time
);

extern "C"
NOINLINE INTERFACE_ATTRIBUTE
ADDRTY __obfuscuro_data_addr_translate(void);

extern "C"
NOINLINE 
unsigned long __obfuscuro_data_addr_translate_inner(void) {
  register ADDRTY req_addr asm ("r15");

  // Adil. Since the first data request has to be related to the
  // stack of the program, we simply populate stack from that address
  // forward as the oblivious stack.
  if (stack_population == 0) {
    //sgx_print_string("[BG] populate stack!\n");
    //sgx_print_hex((unsigned long) req_addr);
    //sgx_print_string("\n");
    populate_stack(req_addr);
    stack_population = 1;
  }

  unsigned long dst_addr = (unsigned long) otranslate(req_addr, __DATA);

  // Sanity check.
  if(dst_addr == 0)
  {
      sgx_print_string("[BG] please fix this!!\n");
      dst_addr = (unsigned long)&scratch_data.buf;
  }

  return dst_addr;
}

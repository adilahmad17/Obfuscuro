#include <stdlib.h>
#include "common.h"
#include "oram.h"

int temp[64];

#define DEBUG_REGISTER 0

void reg_move_to_stash(uint64_t addr, int index) {

#if DEBUG_REGISTER == 1
  uint64_t t = addr;
  for (int i = 0; i < 64/sizeof(int); i++) {
    int* tmp = (int*) t;
    printf("%02x ", *tmp);
    t += sizeof(int);
  }
  printf("\n");
#endif

  if (index == 0) {
    __asm__ (
        "mov %0, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm5, %%ymm5;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm5, %%ymm5;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm6, %%ymm6;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm6, %%ymm6;"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm5", "ymm6", "xmm0"
    );
  } else if (index == 1) {
    __asm__ (
        "mov %0, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm7, %%ymm7;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm7, %%ymm7;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm8, %%ymm8;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm8, %%ymm8;"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm7", "ymm8", "xmm0"
    );
  } else if (index == 2) {
    __asm__ (
        "mov %0, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm9, %%ymm9;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm9, %%ymm9;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm10, %%ymm10;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm10, %%ymm10;"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm9", "ymm10", "xmm0"
    );
  } else if (index == 3) {
    __asm__ (
        "mov %0, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm11, %%ymm11;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm11, %%ymm11;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm12, %%ymm12;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm12, %%ymm12;"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm11", "ymm12", "xmm0"
    );
  } else if (index == 4) {
    __asm__ (
        "mov %0, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm13, %%ymm13;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm13, %%ymm13;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm14, %%ymm14;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm14, %%ymm14;"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm13", "ymm14", "xmm0"
    );
  } else if (index == 5) {
    __asm__ (
        "mov %0, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm15, %%ymm15;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm15, %%ymm15;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm2, %%ymm2;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm2, %%ymm2;"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm15", "ymm4", "xmm0"
    );
  } else if (index == 6) {
    __asm__ (
        "mov %0, %%rsi;"
        "movaps (%%rsi), %%xmm1;"
        "vinserti128 $0x0, %%xmm1, %%ymm0, %%ymm0;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm1;"
        "vinserti128 $0x1, %%xmm1, %%ymm0, %%ymm0;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm1, %%ymm1;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm1, %%ymm1;"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm0", "ymm1", "xmm0", "xmm1"
    );
  } else if (index == 7) {
    __asm__ (
        "mov %0, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm3, %%ymm3;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm3, %%ymm3;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x0, %%xmm0, %%ymm4, %%ymm4;"
        "add $16, %%rsi;"
        "movaps (%%rsi), %%xmm0;"
        "vinserti128 $0x1, %%xmm0, %%ymm4, %%ymm4;"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm3", "ymm4", "xmm0"
    );
  }

}

void reg_copy_from_stash(uint64_t addr, int index) {
  if (index == 0) {
    __asm__ (
        "mov %0, %%rsi;"
        "vextracti128 $0x0, %%ymm5, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm5, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x0, %%ymm6, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm6, (%%rsi)\n"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm5", "ymm6"
    );
  } else if (index == 1) {
    __asm__ (
        "mov %0, %%rsi;"
        "vextracti128 $0x0, %%ymm7, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm7, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x0, %%ymm8, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm8, (%%rsi)\n"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm7", "ymm8"
    );
  } else if (index == 2) {
    __asm__ (
        "mov %0, %%rsi;"
        "vextracti128 $0x0, %%ymm9, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm9, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x0, %%ymm10, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm10, (%%rsi)\n"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm9", "ymm10"
    );
  } else if (index == 3) {
    __asm__ (
        "mov %0, %%rsi;"
        "vextracti128 $0x0, %%ymm11, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm11, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x0, %%ymm12, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm12, (%%rsi)\n"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm11", "ymm12"
    );
  } else if (index == 4) {
    __asm__ (
        "mov %0, %%rsi;"
        "vextracti128 $0x0, %%ymm13, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm13, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x0, %%ymm14, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm14, (%%rsi)\n"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm13", "ymm14"
    );
  } else if (index == 5) {
    __asm__ (
        "mov %0, %%rsi;"
        "vextracti128 $0x0, %%ymm15, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm15, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x0, %%ymm2, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm2, (%%rsi)\n"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm15", "ymm2"
    );
  } else if (index == 6) {
    __asm__ (
        "mov %0, %%rsi;"
        "vextracti128 $0x0, %%ymm0, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm0, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x0, %%ymm1, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm1, (%%rsi)\n"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm0", "ymm1"
    );
  } else if (index == 7) {
    __asm__ (
        "mov %0, %%rsi;"
        "vextracti128 $0x0, %%ymm3, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm3, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x0, %%ymm4, (%%rsi)\n"
        "add $16, %%rsi;"
        "vextracti128 $0x1, %%ymm4, (%%rsi)\n"
      :
      : "m" (addr)
      : "cc", "rsi", "ymm3", "ymm4"
    );
  }
}

void debug_stash(int index) {
  DOUT("DEBUGGING STASH START\n");

  DOUT("Copying from stash\n");
  reg_copy_from_stash((uint64_t) temp, index);

  DOUT("register index: %d\n", index);
  for (int i = 0; i < 64/sizeof(int); i++) {
    DOUT("%02x ", temp[i]);
  }
  DOUT("\n");

  DOUT("Writing back to stash\n");
  reg_move_to_stash((uint64_t) temp, index);

  DOUT("DEBUGGING STASH END\n");
}
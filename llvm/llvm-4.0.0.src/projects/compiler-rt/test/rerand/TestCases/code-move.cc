// RUN: %clangxx -O0 %s -o %t1
// RUN: %clangxx_rerand_code -O0 %s -o %t2
// RUN: %clangxx_rerand -O0 %s -o %t3

// RUN: %env_rerand_opts %run %t1 2>&1 | FileCheck %s --check-prefix=PCHECK
// RUN: %env_rerand_opts %run %t2 2>&1 | FileCheck %s --check-prefix=NCHECK
// RUN: %env_rerand_opts %run %t3 2>&1 | FileCheck %s --check-prefix=NCHECK

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sanitizer/rerand_interface.h>

bool foo() {
  printf("[*] func foo()\n");
  return true;
}

bool bar() {
  printf("[*] func bar()\n");
  return false;
}

void* my_memcpy(void* dest, const void* src, size_t count) {
  char* dst8 = (char*)dest;
  char* src8 = (char*)src;
  while (count--) {
    *dst8++ = *src8++;
  }
  return dest;
}

char simple_loop(const void* src, size_t count) {
  char dst;

  char* src8 = (char*)src;
  while (count--) {
    dst += *src8++;
  }
  return dst;
}

void testme() {
  bool (*funcptr) ();
  bool res;

  long src[1024];
  long dst[1024];
  for (int i=0; i<1024; i++) {
    my_memcpy(dst, src, 1024);
    simple_loop(src, 1024);
  }

  funcptr = foo;
  res = (*funcptr)();
  assert(res == true);

  funcptr = bar;
  res = (*funcptr)();
  assert(res == false);
}

void shuffle() {
#ifdef __RERAND_CODE__
  unsigned long aligned = ((unsigned long)foo >> 12) << 12;
  unsigned long size = (unsigned long)shuffle - (unsigned long)foo;

  int prot = PROT_READ|PROT_WRITE|PROT_EXEC;
  int flags = MAP_PRIVATE|MAP_ANON;
  void *code_block = mmap(0, 4096, prot, flags, -1,0);

  if (code_block == (void*)-1) {
    perror("[-] mmap failed");
    exit(-1);
  }

  printf("[*] code_block : %p\n", code_block);
  printf("[*] size : %lx\n", size);

  // TODO: Estimating the size of foo function.
  unsigned long addr1 = (unsigned long)foo;
  unsigned long addr2 = (unsigned long)code_block;
  __rerand_move(addr1, addr2, size);
#endif
}

int main(void) {
  printf("[*] testme() address: %p\n", testme);
  printf("[*] printf() address: %p\n", printf);

  testme();
  printf("[*] Before shuffle\n");

  shuffle();
  printf("[*] After shuffle\n");

  testme();


  // NCHECK: [RT] (__rerand_code_call
  // NCHECK: {{end of main}}
  // PCHECK: {{end of main}}
  printf("[*] end of main\n");
  return 0;
}

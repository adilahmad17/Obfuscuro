// RUN: %clangxx -O0 %s -o %t1
// RUN: %clangxx_rerand_data -O0 %s -o %t2
// RUN: %clangxx_rerand -O0 %s -o %t3

// RUN: %env_rerand_opts %run %t1 2>&1 | FileCheck %s --check-prefix=PCHECK
// RUN: %env_rerand_opts %run %t2 2>&1 | FileCheck %s --check-prefix=NCHECK
// RUN: %env_rerand_opts %run %t3 2>&1 | FileCheck %s --check-prefix=NCHECK

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sanitizer/rerand_interface.h>

#define NUM_DATA 2
typedef struct S {
  int v1;
  int v2;
} S;

S global[NUM_DATA];

void test(S *global_alias) {
  int local[NUM_DATA];

  printf("[*] local range: %p - %p\n", &local[0], &local[NUM_DATA]);
  printf("[*] global range: %p - %p\n", &global_alias[0], &global_alias[NUM_DATA]);

  for (int i=0; i<NUM_DATA; i++) {
    local[i] = i*2;
    global_alias[i].v1 = i;
    global_alias[i].v2 = i*2;
  }

#ifdef __RERAND_DATA__
  unsigned long size = ((unsigned long)&global_alias[NUM_DATA] - (unsigned long)&global_alias[0])/2;
  unsigned long addr1 = (unsigned long)&global_alias[0];
  unsigned long addr2 = addr1 + size;
  __rerand_shuffle(addr1, addr2, size);
#endif

  for (int i=0; i<NUM_DATA; i++) {
    assert(local[i] == i*2);
  }

  for (int i=0; i<NUM_DATA; i++) {
    if (global_alias[i].v1 != i) {
      printf("[-] Fail in global_alias[%d].v1\n", i);
      assert(false);
    }
    if (global_alias[i].v2 != i*2) {
      printf("[-] Fail in global_alias[%d].v2\n", i);
      assert(false);
    }
    printf("[*] OK i: %d\n", i);
  }
}


int main(void) {
  for (int i=0; i<5; i++) {
    test(global);
  }

  // NCHECK: [RT] (__rerand_addr_translate
  // NCHECK: {{end of main}}
  // PCHECK: {{end of main}}
  printf("[*] end of main\n");
  return 0;
}

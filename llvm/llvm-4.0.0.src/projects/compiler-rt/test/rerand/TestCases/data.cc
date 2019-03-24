// RUN: %clangxx -O0 %s -o %t1
// RUN: %clangxx_rerand_data -O0 %s -o %t2
// RUN: %clangxx_rerand -O0 %s -o %t3

// RUN: %env_rerand_opts %run %t1 2>&1 | FileCheck %s --check-prefix=PCHECK
// RUN: %env_rerand_opts %run %t2 2>&1 | FileCheck %s --check-prefix=NCHECK
// RUN: %env_rerand_opts %run %t3 2>&1 | FileCheck %s --check-prefix=NCHECK

#include <stdio.h>
#include <assert.h>
#include <sanitizer/rerand_interface.h>

#define NUM_DATA 1024
int global[NUM_DATA];

void testme(int *local) {
#ifdef __RERAND_DATA__
  {
    unsigned long size;
    unsigned long addr1;
    unsigned long addr2;
    size = ((unsigned long)&global[NUM_DATA] - (unsigned long)&global[0])/2;
    addr1 = (unsigned long)&global[0];
    addr2 = addr1 + size;
    __rerand_shuffle(addr1, addr2, size);

    // size = ((unsigned long)&local[NUM_DATA] - (unsigned long)&local[0])/2;
    // addr1 = (unsigned long)&local[0];
    // addr2 = addr1 + size;
    // __rerand_shuffle(addr1, addr2, size);
  }
#endif

  for (int i=0; i<NUM_DATA; i++) {
    if (global[i] != i) {
      printf("[-] Fail in i: %d\n", i);
      assert(false);
    }
  }

  for (int i=0; i<NUM_DATA; i++) {
    if (local[i] != i*2) {
      printf("[-] Fail in i: %d\n", i);
      assert(false);
    }
  }
  return;
}

int main(void) {
  int local[NUM_DATA];

  printf("[*] local range: %p - %p\n", &local[0], &local[NUM_DATA]);
  printf("[*] global range: %p - %p\n", &global[0], &global[NUM_DATA]);

  for (int i=0; i<NUM_DATA; i++) {
    local[i] = i*2;
    global[i] = i;
  }

  for (int i=0; i<10; i++) {
    printf("[*] Iteration %d\n", i);
    testme(local);
  }
  // NCHECK: [RT] (__rerand_addr_translate
  // NCHECK: {{end of main}}
  // PCHECK: {{end of main}}
  printf("[*] end of main\n");
  return 0;
}


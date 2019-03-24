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
#include <sanitizer/rerand_interface.h>
#include <sys/mman.h>

void numsift(long *array, unsigned long i, unsigned long j) {
  long temp;

  while(i<=j) {
    if(array[i]>=array[i+1]) {
      i=j+1;
    }
  }
  return;
}

void testme() {
  long array[100];
  memset(array, 0, sizeof(array));
  numsift(array, 10, 90);
}

int main(void) {
  printf("[*] testme() address: %p\n", testme);
  printf("[*] printf() address: %p\n", printf);

  testme();
  printf("[*] end of main\n");
  // NCHECK: [RT] (__rerand_code_call
  // NCHECK: {{end of main}}
  // PCHECK: {{end of main}}
  return 0;
}

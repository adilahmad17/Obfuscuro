#include <stdlib.h>
#include "common.h"
#include "oram.h"

/* get a random number (32-bit) */
int get_rand32(unsigned int* rand) {
  unsigned char err;
  asm volatile("rdrand %0 ; setc %1"
                : "=r" (*rand), "=qm" (err));
  return (int) err;
}

/* find if in path or not */
// ISSUE. Make this oblivious
int is_in_path(int leaf, int path_index) {
  if (leaf == path_index) return 1;

  if (path_index > leaf) return 0;

  while (true) {
    if (path_index > leaf) break;
    else if (path_index == leaf) return 1;

    leaf -= 1;
    leaf /= 2;
  }

  return 0;
}
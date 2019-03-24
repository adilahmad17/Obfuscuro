#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

#define SIZE 160

// C function to find maximum in arr[] of size n
int arr[SIZE];
int index = 0;
int ret[16];

void prog_term(void)
{
#ifdef RERAND
  int check = -1;
  __obfuscuro_fetch((unsigned long) &check, (unsigned long) &ret, sizeof(int));
  ocall_exit(&check, sizeof(int));
#endif
}

extern "C"
void sum() __attribute__((oblivious))
{
  unsigned int i = 0;
  ret[0] = 0;
  for(i=0; i < SIZE; ++i)
  {
    ret[0] += arr[i];   // sum = sum+i;
  }
#ifdef RERAND
  indefinite_execution();
#else
  eprintf("Answer = %d\n", ret[0]);
#endif
}

void entry(void)
{
  for (int i = 0; i < SIZE; i++) {
    arr[i] = i;
  }
  int n = sizeof(arr)/sizeof(arr[0]);

#ifdef RERAND
  eprintf("------- POPULATION -------\n");
  populate_code_oram((void *) &sum, "sum");
  populate_code_oram((void *) &indefinite_execution, "indefinite_execution");
  for(int i=0; i<SIZE*sizeof(int); i+=64)
  {
    eprintf("populating data at %p\n", ((unsigned long int) arr) + i);
    __obfuscuro_populate(((unsigned long int) arr) + i, 64, __DATA);
  }
  __obfuscuro_populate(((unsigned long int) ret), 64, __DATA);

  eprintf("------- POPULATION -------\n");
  eprintf("------- DEBUG -------\n");
  eprintf("ADDRESS OF RET: %p\n", (unsigned long) &ret);
  eprintf("ADDRESS OF ARR: %p\n", (unsigned long) &arr);
  eprintf("------- DEBUG -------\n");
#endif

  eprintf("Starting the enclave execution\n");
  eprintf("Program: Sum\n");
  sum();
}

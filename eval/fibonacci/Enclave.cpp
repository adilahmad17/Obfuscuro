#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

int global_output[16];
int return_output;
int answer = -1;

void prog_term(void)
{
#ifdef RERAND
  __obfuscuro_fetch((unsigned long) &answer, (unsigned long) &global_output, sizeof(int));
  ocall_exit((void*) &answer, sizeof(int));
#endif
}

extern "C"
void f(int n) __attribute__((oblivious))
{
  int cur = 0;
  int next = 1;
  int count = 0;

  for (int i = 2; i <= n; i++) {
    global_output[0] = cur + next;
    cur = next;
    next = global_output[0];
  }

#ifdef RERAND
  indefinite_execution();
#else
  eprintf("Answer = %d\n", global_output[0]);
#endif
}

void entry(void)
{
#ifdef RERAND
  populate_code_oram((void *)&f, "f");
  populate_code_oram((void *)&indefinite_execution, "indefinite_execution");
  __obfuscuro_populate((unsigned long) &global_output, 64, __DATA);
#endif

  eprintf("Starting the enclave execution\n");
  eprintf("Program: Fibonacci\n");
  f(8);
}

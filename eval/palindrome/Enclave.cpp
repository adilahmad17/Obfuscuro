#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

#define SIZE 160

// C function to find maximum in arr[] of size n
int index = 0;
int ret[16];
//unsigned int reversedInteger = 0;

void prog_term(void)
{
#ifdef RERAND
  int check = -1;
  __obfuscuro_fetch((unsigned long) &check, (unsigned long) &ret, sizeof(int));
  ocall_exit(&check, sizeof(int));
#endif
}

extern "C"
void palindrome()__attribute__((oblivious))
{
    unsigned int n, remainder, originalInteger;
    unsigned int reversedInteger = 0;
    n = 1234321;
    originalInteger = n;
    int* ret_inner = (int*) &ret;

    while( n!=0 )
    {
      remainder = n%10;
      reversedInteger = reversedInteger*10 + remainder;
      n /= 10;
    }

    if (originalInteger == reversedInteger) {
       *ret_inner = 1;
    } else {
       *ret_inner = 0;
    }
#ifdef RERAND
  indefinite_execution();
#else
  eprintf("Answer = %d\n", *ret_inner);
#endif
}

void entry(void)
{
#ifdef RERAND
  eprintf("------- POPULATION -------\n");
  populate_code_oram((void *) &palindrome, "palindrome");
  populate_code_oram((void *) &indefinite_execution, "indefinite_execution");
  __obfuscuro_populate((unsigned long) &ret, 64, __DATA);
#endif

  eprintf("Starting the enclave execution\n");
  eprintf("Program: Palindrome (1 for yes, 0 for no)\n");
  palindrome();
  eprintf("Palindrome End\n" );
}

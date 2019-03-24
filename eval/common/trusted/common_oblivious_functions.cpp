#include "Enclave.h"
#include "common_oblivious_functions.h"

extern "C"
bool assert_input(void* memory, unsigned int expected) __attribute__((oblivious))
{
  unsigned int* check = (unsigned int*) memory;
  if (*check == expected) return true;
  return false;
}

extern "C"
void indefinite_execution(void) __attribute__((oblivious))
{
    while(true);
}

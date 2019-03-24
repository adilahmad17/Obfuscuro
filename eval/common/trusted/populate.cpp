#ifdef RERAND
#include "reflect.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "Enclave.h"

bool startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

void strcat(char * a, char *b)
{
  //find end of a
  int i=0;
  while(a[i] != 0)
  {
    i++;
  }
  int j=0;
  while(b[j] !=0)
  {
    a[i++] = b[j++];
  }
  a[i] = '\0';

}

extern "C"
void populate_code_oram(void * function_addr, char * function_name)
{
  eprintf("populating code from target program \n");

  char buffer[256];
  char bufferWithoutNum[256];

  memset(buffer, 0, 256);
  memset(bufferWithoutNum, 0, 256);

  strcat(buffer, "__OBFUSCURO_OBLIVIOUS_LABEL_");
  strcat(buffer, function_name);
  strcat(buffer, ".");
  strcat(buffer, "0");

  strcat(bufferWithoutNum, "__OBFUSCURO_OBLIVIOUS_LABEL_");
  strcat(bufferWithoutNum, function_name);
  strcat(bufferWithoutNum, ".");

  eprintf("searching! : %s\n", buffer);
  int symbol_index = return_symbol_index(buffer);
  if(symbol_index == -1)
  {
    eprintf("[ERROR] symbol_index == -1\n");
  }

  unsigned long zero_addr = (unsigned long)gbl_sym_table[symbol_index].addr;

  assert(symbol_index != -1);
  int j = 0;

  while(startsWith(bufferWithoutNum, gbl_sym_table[symbol_index + j].name))
  {

    unsigned long pop_addr = (unsigned long)function_addr + (unsigned long)gbl_sym_table[symbol_index + j].addr - zero_addr;
    __obfuscuro_populate((unsigned long) pop_addr, 64, __CODE);

    //sanity check
    unsigned char * tmp_bb = (unsigned char *)((unsigned long)pop_addr);
    if(startsWith(bufferWithoutNum, "__OBFUSCURO_OBLIVIOUS_LABEL_indefinite_execution.") == false)
    {
        eprintf("populating %s, %p => ",gbl_sym_table[symbol_index + j].name, pop_addr );
        if(tmp_bb[61] != 0x41 || tmp_bb[62] != 0xff || tmp_bb[63] != 0xe6)
        {
          eprintf("%x, %x, %x\n",tmp_bb[61],tmp_bb[62],tmp_bb[63] );
          eprintf("sanity check fail\n");
          // Adil: I added this on purpose, shouldn't come here
          assert(false);
        }else
        {
            eprintf("sanity check passed\n");
        }
    }
    j++;
  }

  eprintf("populate_code_oram_called, %p\n", function_addr);

}
#endif

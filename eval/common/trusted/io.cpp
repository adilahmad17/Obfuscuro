#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include "io.h"
#include "assert.h"
#include "Enclave_t.h"  /* print_string */
#include "Enclave.h"


#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

void eprintf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

void itoa(unsigned long num, char *str, int radix){ 
static char digit[16]={'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    int i=0; 
    unsigned long deg=1; 
    int cnt = 0; 
    while(1){  
        if( (num/deg) > 0) 
            cnt++; 
        else 
            break; 
        deg *= radix; 
    } 
    deg /=radix;    
    for(i=0; i<cnt; i++)    {    
        *(str+i) = digit[num/deg];  
        num -= ((num/deg) * deg);  
        deg /=radix;    
     }
    *(str+i) = '\0';    

}

void sgx_print_string(const char * str)
{
  ocall_print_string(str);
}

void sgx_print_integer(unsigned long d)
{
   char buf[32];
   itoa(d, buf, 10);
   ocall_print_string(buf);
}

void sgx_print_32_bit_hex(unsigned int d)
{
   char buf[32]; 
   itoa(d, buf, 16);
   ocall_print_string(buf);
}

void sgx_print_hex(unsigned long d)
{
  unsigned int d2= (unsigned int)((d>>32)&0xffffffff); 
  sgx_print_32_bit_hex(d2);
  unsigned int d1= (unsigned int)(d&0xffffffff); 
  sgx_print_32_bit_hex(d1);
}

// This function simply reads the whole executable file
void* sgx_read_executable(size_t* size) {

  eprintf("Reading the symbols from shared file\n");

  // XXX. Make generic
  char filename[] = "enclave.with.symbol.so";

  // 2. Read the size of this file
  unsigned long filesize = 0;
  ocall_getsize(&filesize, filename);
  if (filesize == 0) assert(false);
  *size = filesize;

  // Make space 
  void* program = malloc(filesize+1);
  if (program == NULL) assert(false);

  int fd;
  ocall_open(&fd, filename);
  if (fd == -1) assert(false);

  size_t ret = 0;
  ocall_read(&ret, program, filesize, fd);
  if (!ret) assert(false);

  eprintf("Reading Symbols --- Complete\n");

  return program;
}

unsigned long get_codepad_addr(void)
{
  unsigned long ret = 0;
  ocall_get_codepad_addr(&ret);
  return ret;
}

unsigned long get_datapad_addr(void)
{
  unsigned long ret = 0;
  ocall_get_datapad_addr(&ret);
  return ret;
}

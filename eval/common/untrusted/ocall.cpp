#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>

#include "sgx_urts.h"
#include "Enclave_u.h"

void* ocall_read_executable(size_t* size)
{
  return NULL;
}

size_t ocall_getsize(char* filename)
{
  struct stat st;
  stat(filename, &st);
  return st.st_size;
}

FILE* ocall_fopen(char* filename, const char* mode)
{
	assert(filename != NULL && mode != NULL);
	printf("file to open: %s, mode: %s\n", filename, mode);
	FILE* f = fopen(filename, mode);
	assert(f != NULL);
	return f;
}

int ocall_fscanf(FILE* f, const char* arg, int* buf)
{
	assert(arg != NULL && f != NULL);
	int ans = fscanf(f, arg, buf);
	return ans;
}

void ocall_fclose(FILE* f)
{
	fclose(f);
}

char* ocall_fgets(char* buf, size_t count, FILE* f)
{
  printf("size to read: %ld\n", count);
  char* ret = fgets(buf, count, f);
  assert(ret != NULL);
  return ret;
}

size_t ocall_fread (void * ptr, size_t size, size_t count, FILE * f) {
  printf("size to read: %ld, from: %ld\n", size, count);
  size_t ret = fread(ptr, size, count, f);
  printf("[OCALL] ret: %d\n", ret);
  return ret;
}

int ocall_open(char* name)
{
  int fd = open(name, O_RDONLY);
  return fd;
}

size_t ocall_read(void* ptr, size_t size, int fd)
{
  size_t ret = read(fd, ptr, size);
  return ret;
}

#include <iostream>
#include <chrono>

typedef std::chrono::high_resolution_clock Clock;
std::chrono::time_point <std::chrono::high_resolution_clock> t1;
std::chrono::time_point <std::chrono::high_resolution_clock> t2;

void ocall_start_timer(void)
{
  t1 = Clock::now();
}

void ocall_end_timer(void)
{
  t2 = Clock::now();
  std::cout << "Time Taken: " << std::chrono::duration_cast<std::chrono::nanoseconds>(t2-t1).count() << std::endl;
}

void ocall_exit(void* ptr, size_t size) {
  printf("Terminating the untrusted program\n");
  if (size == sizeof(int)) {
    int* integer = (int*) ptr;
    printf("The enclave returned: %d\n", *integer);
  }
  exit(0);
}

unsigned long ocall_get_codepad_addr(void){return m_codepad_addr;}
unsigned long ocall_get_datapad_addr(void){return m_datapad_addr;}

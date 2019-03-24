/* This header file contains the declarations for various variables used by the
 * enclave to provide oram functionality 

   Date: Apr 5, 2017
 */

#ifndef ISGX_ORAM
#define ISGX_ORAM

#include <linux/types.h>
#include <linux/ioctl.h> 

#define SGX_SIGFAULT			50

#define MAXENCLAVES                     5
#define MAXPAGES                        10000

extern void* tmp_ocall_table;
extern void* tmp_ms;

struct enclave {
	__u64			addr;
	unsigned long 		pageTable[MAXPAGES];
	int 			filled;
};

extern struct enclave enclaveTab[MAXENCLAVES];

void sgx_contact_enclave(unsigned long addr);
#endif


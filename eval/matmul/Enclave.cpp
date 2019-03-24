#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

// C program to multiply two square matrices.
#include <stdio.h>
#define N 4

unsigned int output[N][N];

unsigned int res[N][N]  = {{0, 0, 0, 0},
                  {0, 0, 0, 0},
                  {0, 0, 0, 0},
                  {0, 0, 0, 0}};

unsigned int mat1[N][N] = {{1, 1, 1, 1},
                  {2, 2, 2, 2},
                  {3, 3, 3, 3},
                  {4, 4, 4, 4}};

unsigned int mat2[N][N] = {{1, 1, 1, 1},
                  {2, 2, 2, 2},
                  {3, 3, 3, 3},
                  {4, 4, 4, 4}};

void prog_term(void)
{
#ifdef RERAND
  __obfuscuro_fetch((unsigned long) &output, (unsigned long) &res, (sizeof(unsigned int)*N*N));
  ocall_exit((&output[1][0]), sizeof(unsigned int));
#endif
}

// This function multiplies mat1[][] and mat2[][],
// and stores the result in res[][]
extern "C"
void multiply(unsigned int mat1[][N], unsigned int mat2[][N]) __attribute__((oblivious))
{
    unsigned int i, j, k;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            res[i][j] = 0;
            for (k = 0; k < N; k++)
                res[i][j] = res[i][j] + mat1[i][k]*mat2[k][j];
        }
    }

#ifdef RERAND
  indefinite_execution();
#endif
}

extern "C"
int check_input(unsigned int mat[][N], int x, int y) __attribute__((oblivious))
{
  return mat[x][y];
}

extern "C"
unsigned long check_address(unsigned int mat[][N]) __attribute__((oblivious))
{
  return (unsigned long) &mat;
}

void entry(void)
{

#ifdef RERAND
  populate_code_oram((void *) &multiply, "multiply");
  populate_code_oram((void *) &check_input, "check_input");
  populate_code_oram((void *) &check_address, "check_address");
  populate_code_oram((void *) &indefinite_execution, "indefinite_execution");

  eprintf("Location for mat1 => (%p-%p)\n", (unsigned long) mat1, ((unsigned long) mat1) + 64);
  eprintf("Location for mat2 => (%p-%p)\n", (unsigned long) mat2, ((unsigned long) mat2) + 64);
  eprintf("Location for res => (%p-%p)\n", (unsigned long) res, ((unsigned long) res) + 64);

  __obfuscuro_populate((unsigned long long int)mat1, 64, __DATA);
  __obfuscuro_populate((unsigned long long int)mat2, 64, __DATA);
  __obfuscuro_populate((unsigned long long int)res, 64, __DATA);

  /* ADIL. TODO. add all debugging code to seperate file */
/*
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      assert((i+1) == check_input(mat1, i, j));
      assert((i+1) == check_input(mat2, i, j));
      assert(0 == check_input(res, i, j));
    }
  }
  eprintf("The address of mat1 (oblivious) is: %p\n", check_address(mat1));
  eprintf("The address of mat2 (oblivious) is: %p\n", check_address(mat2));
  eprintf("The address of res (oblivious) is: %p\n", check_address(res));
*/

#endif

  eprintf("Starting the enclave execution\n");
  eprintf("Program: Matmul\n");
  int i, j;

  multiply(mat1, mat2);

#ifdef RERAND
  eprintf("Fetching the output ... \n");
  __obfuscuro_fetch((unsigned long) &output, (unsigned long) &res, (sizeof(int)*N*N));

  eprintf("Result matrix is \n");
  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
      eprintf("%d ", output[i][j]);
    eprintf("\n");
  }
#else
  eprintf("Result matrix is \n");
  for (i = 0; i < N; i++)
  {
    for (j = 0; j < N; j++)
      eprintf("%d ", res[i][j]);
    eprintf("\n");
  }
#endif
}

#include <stdio.h>

struct sym_table_t {
    const char * name;
    void * addr;
};

int return_symbol_index(const char * name);
void *reflect_query_symbol(const char * name);

extern struct sym_table_t gbl_sym_table[];

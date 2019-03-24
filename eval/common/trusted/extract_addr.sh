#!/bin/bash -e

nm $1 |awk 'BEGIN{ print "#include \"reflect.h\""; print "struct sym_table_t gbl_sym_table[]={";} {if(NF==3){ print "{\"" $3 "\", (void *)0x" $1 "}, ";}} END{print "{NULL, NULL}};";}'

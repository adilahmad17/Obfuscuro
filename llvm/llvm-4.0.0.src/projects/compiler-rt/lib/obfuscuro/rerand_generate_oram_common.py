#!/bin/bash/python
templete_file = open('./rerand_oram_common.h');

source_code= templete_file.read()
template = source_code.replace("#define NUM_TREE_LEAF", "#define NUM_TREE_LEAF %d //");
template = template.replace("#define NUM_STASH_BLOCKS", "#define NUM_STASH_BLOCKS %d //");


for leaf_num in [128, 512, 1024, 2048, 4096, 8192]:
    for stash_size in [50, 100, 200, 400, 800, 1600]:
        output_file = open("./tmp_oram_common/rerand_oram_common_%d_2_%d.h" % (leaf_num, stash_size), "w")
        output_file.write(template % (leaf_num, stash_size))
        output_file.close()


templete_file.close()

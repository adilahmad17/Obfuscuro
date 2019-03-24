/* Contains definitions for ORAM implementation for rerandomization */

#include <stdlib.h>
#include "common.h"
#include "oram.h"

/* required data structures for code-based ORAM */
tree_block_t ostash_code[NUM_STASH_CODE_BLOCKS];
tree_node_t otree_code[NUM_TREE_CODE_NODES];
pmap_block_t oposmap_code[NUM_TREE_CODE_LEAF];
smap_block_t osmap_code[NUM_STASH_CODE_BLOCKS];
tmap_block_t otmap_code[NUM_TREE_CODE_NODES];

/* required data structures for code-based ORAM */
tree_block_t ostash_data[NUM_STASH_DATA_BLOCKS];
tree_node_t otree_data[NUM_TREE_DATA_NODES];
pmap_block_t oposmap_data[NUM_TREE_DATA_LEAF];
smap_block_t osmap_data[NUM_STASH_DATA_BLOCKS];
tmap_block_t otmap_data[NUM_TREE_DATA_NODES];

/* dummy memory */
char dummy_memory[PAGE_SIZE];

/* count of filled oram blocks */
int count_filled_code_blocks = 0;
int count_filled_data_blocks = 0;

void update_position_map(ADDRTY addr, SIZETY size, int leaf, int pos, OTYPE type) {
  pmap_block_t* temp;
  ADDRTY n_addr;
  int loop_end;

  if (type == __CODE) {
    n_addr = (ADDRTY) &(otree_code[pos].blocks[0].memory);
    loop_end = NUM_TREE_CODE_LEAF;
  } else {
    n_addr = (ADDRTY) &(otree_data[pos].blocks[0].memory);
    loop_end = NUM_TREE_DATA_LEAF;
  }

  int to_check;

  for (int i = 0; i < loop_end; i++) {

    /* check the type of update */
    if (type == __CODE) {
      if (i == 0) DOUT("UPDATE: code-based pmap\n");
      temp = &(oposmap_code[i]);
      to_check = count_filled_code_blocks;
    } else {
      if (i == 0) DOUT("UPDATE: data-based pmap\n");
      temp = &(oposmap_data[i]);
      to_check = count_filled_data_blocks;
    }

    if (to_check == i) {
      temp->leaf = leaf;
      temp->old_addr = addr;
      temp->new_addr = n_addr;
      temp->size = size;
      temp->type = type;
#if SEC == 0
      return;
#endif
    } else {
#if SEC > 0
      // using cmov
      temp->leaf = cmov(temp->leaf, 1, leaf);
      temp->old_addr = cmov(temp->old_addr, 1, addr);
      temp->new_addr = cmov(temp->new_addr, 1, n_addr);
      temp->size = cmov(temp->size, 1, size);
      temp->type = (OTYPE) cmov(temp->type, 1, type);
#endif
    }

  }

  return;
}

int populate_tree_using_memory(ADDRTY addr, SIZETY size, int leaf, OTYPE type) {
  int flag = 0;
  int ret = -1;
  void* tmp = (void*) addr;
  void* tmp_block;
  tmap_block_t* tmblock;

  int loop_end;
  if (type == __CODE) {
    loop_end = NUM_TREE_CODE_NODES - 1;
  } else {
    loop_end = NUM_TREE_DATA_NODES - 1;
  }

  CHECK(size <= PAGE_SIZE);

  for (int i = loop_end; i >= 0; i--) {

    // check the type to update accordingly
    if (type == __CODE) {
      tmblock = &(otmap_code[i]);
    } else {
      tmblock = &(otmap_data[i]);
    }

    if (tmblock->filled == 0 && is_in_path(leaf, i) && flag == 0) {
      DOUT("chosen: %d, copying: %ld\n", i, size);
      // tmp_block = (void*) &(otree[i].blocks[0].memory);

      if (type == __CODE) {
        tmp_block = (void*) &(otree_code[i].blocks[0].memory);
      } else {
        tmp_block = (void*) &(otree_data[i].blocks[0].memory);
      }

#if SEC > 0
      // cmov_memory
      cmov_memory((char*) tmp_block,(char*) tmp, size, 0);
#elif SEC == 0
      // insecure version
      internal_memcpy(tmp_block, tmp, size);
#endif

      tmblock->filled = 1;
      ret = i;
      flag = 1;

#if SEC == 0
      return ret;
#endif

    } else {
#if SEC > 0
      tmblock->filled = cmov(tmblock->filled, 1, 1);
      ret = (int) cmov(ret, 1, i);
      flag = (int) cmov(flag, 1, 1);

      if (type == __CODE) {
        tmp_block = (void*) &(otree_code[i].blocks[0].memory);
      } else {
        tmp_block = (void*) &(otree_data[i].blocks[0].memory);
      }

      cmov_memory((char*) tmp_block, (char*) tmp, size, 1);
#endif
    }
  }

  CHECK(flag == 1);

  return ret;
}

int check_exist_in_pmap(ADDRTY old_addr, OTYPE type)
{
  pmap_block_t* tmp;

  int loop_end;
  if (type == __CODE) {
    loop_end = NUM_TREE_CODE_LEAF;
  } else {
    loop_end = NUM_TREE_DATA_LEAF;
  }

  for (int i = 0; i < loop_end; i++) {

    if (type == __CODE) {
      if (i == 0) DOUT("FINDING: code-based pmap\n");
      tmp = &(oposmap_code[i]);
    } else {
      if (i == 0) DOUT("FINDING: data-based pmap\n");
      tmp = &(oposmap_data[i]);
    }

    if (tmp->leaf > 0) {
      if (tmp->old_addr == old_addr) {
        // XXX.
        return 1;
      } else if ((old_addr <= tmp->old_addr) && ((tmp->old_addr - PAGE_SIZE) < old_addr)){
        // XXX.
        return 1;
      }
    }
  }

  return 0;
}

/* add a memory region into the oram tree */
void populate_oram(ADDRTY addr, SIZETY size, OTYPE type) {
  /* oram-tree population */

  unsigned int randleaf = 0;
  int pos;

  // DOUT("sanity check passed\n");
  CHECK(size <= PAGE_SIZE);

  if (type == __DATA) {
    //SGXOUT("data variable\n");
    DOUT("data variable\n");
    int ret = check_exist_in_pmap(addr, type);
    if (ret == 1) {
      //SGXOUT("Exists: Yes\n");
      DOUT("Exists: Yes\n");
      return;
    }

    DOUT("Exists: No\n");
  }

  //SGXOUT("old_addr -> %p, size -> %ld\n", addr, size);
  DOUT("old_addr -> %p, size -> %ld\n", addr, size);


  int loop_end;
  if (type == __CODE) {
    loop_end = NUM_TREE_CODE_LEAF;
  } else {
    loop_end = NUM_TREE_DATA_LEAF;
  }

  while (randleaf < loop_end || randleaf >= ((2*loop_end) - 1)) {
    get_rand32(&randleaf);
    randleaf = (randleaf % loop_end);
    randleaf += (loop_end);
  }

  DOUT("Selected leaf: %d\n", randleaf);
  CHECK(randleaf >= loop_end && randleaf < ((2*loop_end) - 1));

  // populate into the right tree
  pos = populate_tree_using_memory(addr, size, randleaf, type);
  CHECK(randleaf != 0);

  // update the right position map
  update_position_map(addr, size, randleaf, pos, type);

  // Sanity check on both trees
  if (type == __CODE) {
    count_filled_code_blocks++;
    CHECK(count_filled_code_blocks <= loop_end);
  } else {
    count_filled_data_blocks++;
    CHECK(count_filled_data_blocks <= loop_end);
  }

  return;
}

void populate_stack(ADDRTY start_addr) {
  unsigned long temp = start_addr;
  for (int i = 0; i < STACK_FRAME_NUM; i++) {
    populate_oram(temp, PAGE_SIZE, __DATA);
    temp -= PAGE_SIZE;
    // sgx_print_hex(temp);
    // sgx_print_string("\n");
  }
}

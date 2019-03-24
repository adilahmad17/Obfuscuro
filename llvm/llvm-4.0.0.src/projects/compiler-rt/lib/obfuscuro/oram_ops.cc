#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "oram.h"
#include <assert.h>

#define NUM_UPDATES 4

// TODO.
// This file is very unclean and contains unnecessary complexity within
// some functions. I will clean it up in the future.

// This is just 64 bytes and safe from cache-level attacks.
typedef struct{
  ADDRTY old_addr;
  ADDRTY new_addr;
}update_queue_t;

update_queue_t update_queue[NUM_UPDATES];
//

// Termination function for each application is prog_term.
extern void prog_term(void);

// Global State
OTYPE cur;

// Variables declared for debugging purposes
static unsigned long int update_pmap_count = 0;
static int timing_or_pattern_experiment = 0;
static unsigned long oramly_translated = 0;
static unsigned int num_executed_code_blocks = 0;
static unsigned int num_fetched_data_blocks = 0;
static unsigned int num_oram_writes = 0;
static unsigned int num_oram_reads = 0;

void update_pmap_addr(ADDRTY old_addr, ADDRTY new_addr, bool pflag) {
  pmap_block_t* pblock;
  int flag = 0;
  bool check = false;
  ADDRTY toadd = 0;

  int loop_end;
  if (cur == __CODE) {
    loop_end = NUM_TREE_CODE_LEAF;
  } else {
    loop_end = NUM_TREE_DATA_LEAF;
  }

  for (int i = 0; i < loop_end; i++) {
    if (cur == __CODE) {
      pblock = &oposmap_code[i];
    } else {
      pblock = &oposmap_data[i];
    }

    check = !((pblock->new_addr == new_addr) && pflag);
    pblock->new_addr = cmov(pblock->new_addr, check, 0);
    flag = cmov(flag, check, 1);
    check = !((pblock->old_addr == old_addr) && pflag);
    pblock->new_addr = cmov(pblock->new_addr, check, new_addr);
    flag = cmov(flag, check, 1);

  }

  //CHECK(flag == 1 || !pflag);
}

void update_pmap_leaf(ADDRTY old_addr, unsigned int new_leaf) {
  pmap_block_t* pblock;
  int flag = 0;
  bool check = false;

  int loop_end;
  if (cur == __CODE) {
    loop_end = NUM_TREE_CODE_LEAF;
  } else {
    loop_end = NUM_TREE_DATA_LEAF;
  }

  for (int i = 0; i < loop_end; i++) {
    if (cur == __CODE) {
      pblock = &oposmap_code[i];
    } else {
      pblock = &oposmap_data[i];
    }

    check = !(pblock->old_addr == old_addr);
    pblock->leaf = cmov(pblock->leaf, check, new_leaf);
    flag = cmov(flag, check, 1);
 }

  CHECK(flag == 1);
}

void write_back_from_scratch_to_stash() {
  tree_block_t* sblock;
  smap_block_t* smblock;
  int flag = 0;
  bool check = false;
  scratchbuf_t* scratchpad;

  int loop_end;
  if (cur == __CODE) {
    scratchpad = &scratch;
    loop_end = NUM_STASH_CODE_BLOCKS;
  } else {
    scratchpad = &scratch_data;
    loop_end = NUM_STASH_DATA_BLOCKS;
  }

  for (int i = 0; i < loop_end; i++) {

    if (cur == __CODE) {
      sblock = &(ostash_code[i]);
      smblock = &(osmap_code[i]);
    } else {
      sblock = &(ostash_data[i]);
      smblock = &(osmap_data[i]);
    }

    check = !(scratchpad->old_addr == smblock->old_addr);
    flag = (flag, check, 1);
    smblock->leaf = cmov(smblock->leaf, check, scratchpad->leaf);
    cmov_memory(sblock->memory, scratchpad->buf, PAGE_SIZE, check);
  }

  CHECK(flag == 1);
}

void fill_tree_node_from_stash_cmov(int path_index) {
  tree_block_t* sblock;
  smap_block_t* smblock;
  char* tblock;
  bool check = false;
  bool check1 = false;
  tmap_block_t* tmblock;
  int cur_queue_fill = 0;

  int loop_end;
  if (cur == __CODE) {
    tblock = (char*) &(otree_code[path_index].blocks[0].memory);
    tmblock = &(otmap_code[path_index]);
    loop_end = NUM_STASH_CODE_BLOCKS;
  } else {
    tblock = (char*) &(otree_data[path_index].blocks[0].memory);
    tmblock = &(otmap_data[path_index]);
    loop_end = NUM_STASH_DATA_BLOCKS;
  }

  int flag = 0;
  for (int i = 0; i < loop_end; i++) {

    if (cur == __CODE) {
      sblock = &(ostash_code[i]);
      smblock = &(osmap_code[i]);
    } else {
      sblock = &(ostash_data[i]);
      smblock = &(osmap_data[i]);
    }

    check = !(smblock->leaf > 0 && smblock->filled == 1 && flag == 0);
    check1 = !(is_in_path(smblock->leaf, path_index));
    smblock->leaf = cmov(smblock->leaf, (check1 || check), 0);
    smblock->filled = cmov(smblock->filled, (check1 || check), 0);
    flag = cmov(flag, check1 || check, 1);
    tmblock->filled = cmov(tmblock->filled, check1 || check, 1);

    // Using the update queue.
    update_queue[cur_queue_fill].old_addr = cmov(update_queue[cur_queue_fill].old_addr, check1||check, smblock->old_addr);
    update_queue[cur_queue_fill].new_addr = cmov(update_queue[cur_queue_fill].new_addr, check1||check, (ADDRTY) tblock);
    cur_queue_fill = cmov(cur_queue_fill, check1||check, cur_queue_fill+1);
    if (cur_queue_fill > 4) {
      sgx_print_string("update queue full\n");
    }
    CHECK(cur_queue_fill <= 4);

    // Without the update queue. (TODO)
    //if (!(check1 || check))
    //update_pmap_addr(smblock->old_addr, (ADDRTY) tblock, !(check1 || check));

    cmov_memory(tblock, sblock->memory, PAGE_SIZE, check1 || check);
    smblock->old_addr = cmov(smblock->old_addr, check1 || check, 0);
    smblock->new_addr = cmov(smblock->new_addr, check1 || check, 0);
  }

  // Stream through the update queue and update the position-map.
  for (int i = 0; i < 4; i++) {
    update_pmap_addr(update_queue[i].old_addr, (ADDRTY) update_queue[i].new_addr, 1);
    update_queue[i].old_addr = 0;
    update_queue[i].new_addr = 0;
  }
}

void write_back_from_stash_to_tree(int old_leaf) {
  int start = old_leaf;
  while (true) {
    fill_tree_node_from_stash_cmov(start);
    if (start == 0) break;
    start -= 1;
    start /= 2;
  }
  return;
}

int check_real(ADDRTY addr, int* leaf, ADDRTY* old_addr) {
  pmap_block_t* pblock;
  int flag = 0;
  bool check = false;
  int loop_end;

  if (cur == __CODE) {
    loop_end = NUM_TREE_CODE_LEAF;
  } else {
    loop_end = NUM_TREE_DATA_LEAF;
  }

  for (int i = 0; i < loop_end; i++) {
    if (cur == __CODE) {
      pblock = &(oposmap_code[i]);
    } else {
      pblock = &(oposmap_data[i]);
    }
    check = !(pblock->leaf > 0 && pblock->new_addr == addr);
    flag = cmov(flag, check, 1);
    *leaf = cmov(*leaf, check, pblock->leaf);
    *old_addr = cmov(*old_addr, check, pblock->old_addr);
  }

  check = !(flag != 1);
  *old_addr = cmov(*old_addr, check, 0xFFFFFF);
  return flag;
}


void find_req_block_from_stash(ADDRTY old_addr, ADDRTY new_addr, int old_leaf) {
  tree_block_t* sblock;
  smap_block_t* smblock;
  char* sblock_mem;
  bool flag = false;
  int leaf;
  scratchbuf_t* scratchpad;
  bool check, check1 = false;
  int loop_end;

  if (cur == __CODE) {
    scratchpad = &scratch;
    loop_end = NUM_STASH_CODE_BLOCKS;
  } else {
    scratchpad = &scratch_data;
    loop_end = NUM_STASH_DATA_BLOCKS;
  }

  for (int i = 0; i < loop_end; i++) {
    if (cur == __CODE) {
      sblock = &(ostash_code[i]);
      smblock = &(osmap_code[i]);
    } else {
      sblock = &(ostash_data[i]);
      smblock = &(osmap_data[i]);
    }

    check = !(smblock->filled == 1);
    check1 = !(smblock->old_addr == old_addr);
    sblock_mem = (char*) (sblock->memory);
    scratchpad->leaf = cmov(scratchpad->leaf, check1 || check, old_leaf);
    scratchpad->new_addr = cmov(scratchpad->new_addr, check1 || check, new_addr);
    scratchpad->old_addr = cmov(scratchpad->old_addr, check1 || check, old_addr);
    flag = cmov(flag, check1 || check, true);
    cmov_memory(scratchpad->buf, sblock_mem, PAGE_SIZE, check1 || check);
  }

  CHECK(flag == true);
}

void cmov_stash_copy(char* block, int old_leaf) {
  tree_block_t* sblock;
  smap_block_t* smblock;
  int fl = 0;
  bool check = false;
  int leaf = old_leaf;
  ADDRTY old_addr;
  int flag = check_real((ADDRTY) block, &leaf, &old_addr);
  int loop_end;

  if (cur == __CODE) {
    loop_end = NUM_STASH_CODE_BLOCKS;
  } else {
    loop_end = NUM_STASH_DATA_BLOCKS;
  }

  for (int i = 0; i < loop_end; i++) {
    if (cur == __CODE) {
      sblock = &(ostash_code[i]);
      smblock = &(osmap_code[i]);
    } else {
      sblock = &(ostash_data[i]);
      smblock = &(osmap_data[i]);
    }

    check = !(smblock->filled == 0 && fl == 0);
    char* sblock_mem = (char*) (sblock->memory);
    cmov_memory(sblock_mem, block, PAGE_SIZE, check);
    smblock->real = cmov(smblock->real, check, flag);
    smblock->leaf = cmov(smblock->leaf, check, leaf);
    smblock->filled = cmov(smblock->filled, check, flag);
    smblock->new_addr = cmov(smblock->new_addr, check, (ADDRTY) block);
    smblock->old_addr = cmov(smblock->old_addr, check, old_addr);
    fl = cmov(fl, check, 1);
  }

  // XXX. Check here for stash being full.
  // CHECK(false);
}

void copy_path_onto_stash(int leaf) {
  int start = leaf;
  char* block;
  tmap_block_t* tmblock;
  while (true) {
    if (cur == __CODE) {
      block = (char*) &(otree_code[start].blocks[0].memory);
      tmblock = &(otmap_code[start]);
    } else {
      block = (char*) &(otree_data[start].blocks[0].memory);
      tmblock = &(otmap_data[start]);
    }
    tmblock->filled = 0;
    cmov_stash_copy(block, leaf);
    if (start == 0) break;
    start -= 1;
    start /= 2;
  }
  return;
}

int locate_addr_from_pmap(ADDRTY old_addr, ADDRTY* closest_addr, ADDRTY* new_addr, 
                          ADDRTY* offset, OTYPE* type) {
  pmap_block_t* tmp;
  int leaf = -1;
  int loop_end;
  bool check = false;
  bool check1 = false;
  bool check2 = false;
  bool check3 = false;

  if (cur == __CODE) {
    loop_end = NUM_TREE_CODE_LEAF;
  } else {
    loop_end = NUM_TREE_DATA_LEAF;
  }

  for (int i = 0; i < loop_end; i++) {

    if (cur == __CODE) {
      tmp = &(oposmap_code[i]);
    } else {
      tmp = &(oposmap_data[i]);
    }

    check = !(tmp->leaf > 0);
    check1 = !(old_addr == 0);
    check2 = !(tmp->old_addr == old_addr);
    check3 = !((old_addr >= tmp->old_addr) && ((tmp->old_addr + tmp->size) > old_addr));
    leaf = cmov(leaf, (check1 || check) && (check2 || check) && (check3 || check), tmp->leaf);
    *new_addr = cmov(*new_addr, (check1 || check) && (check2 || check) && (check3 || check), tmp->new_addr);
    *offset = cmov(*offset,(check1 || check) && (check2 || check), 0);
    *offset = cmov(*offset, (check3 || check), old_addr - tmp->old_addr);
    *closest_addr = cmov(*closest_addr, (check1 || check) && (check2 || check) && (check3 || check), tmp->old_addr);
    *type = (OTYPE) cmov(*type, (check1 || check) && (check2 || check) && (check3 || check), tmp->type);
  }

  return leaf;
}

#define N 2000

unsigned int __rerand_get_executed_code_blocks()
{
    return num_executed_code_blocks;
}
unsigned int __rerand_get_fetched_data_blocks()
{
    return num_fetched_data_blocks;
}

bool allow_output = false;
static long long int time_tick = 0;
ADDRTY otranslate(ADDRTY old_addr, OTYPE provided_type) {
  cur = provided_type;
  if (allow_output == true) {
    allow_output = false;
    goto fetch_block;
  }

  if (cur == __CODE && num_executed_code_blocks > N) {
    sgx_print_string("[*] N blocks executed, returning to ");
    sgx_print_hex((ADDRTY)&prog_term);
    sgx_print_string("\n");
    allow_output = true;
    return (ADDRTY) &prog_term;
  }

fetch_block:

  scratchbuf_t* scratchpad;

  if (cur == __CODE) {
      scratchpad = &scratch;
      num_executed_code_blocks += 1;
#ifdef DEBUG
      sgx_print_string("[BG] code request!:");
      sgx_print_hex(old_addr);
      sgx_print_string("\n");
#endif
  } else {
      scratchpad = &scratch_data;
      num_fetched_data_blocks += 1;
#ifdef DEBUG
      sgx_print_string("[BG] data request!:");
      sgx_print_hex(old_addr);
      sgx_print_string("\n");
#endif
  }

  if (scratchpad->leaf > 0) {
    num_oram_writes++;
    int old_leaf = scratchpad->leaf;
    unsigned int randleaf = 1;
    int loop_end;
    if (cur == __CODE) {
      loop_end = NUM_TREE_CODE_LEAF;
    } else {
      loop_end = NUM_TREE_DATA_LEAF;
    }

    // Find a new leaf for this block
    int loop = 0;
    while (randleaf < loop_end || randleaf >= (2*loop_end -1)) {
     get_rand32(&randleaf);
     randleaf = (randleaf % loop_end);
     randleaf += (loop_end);
     loop++;
    }
    //CHECK(loop == 1);
    CHECK(randleaf >= loop_end && randleaf < (2*loop_end - 1));

    // Update the PMAP (Oblivious)
    scratchpad->leaf = randleaf;
    update_pmap_leaf(scratchpad->old_addr, randleaf);

    // Write back from scratchpad to the stash (Oblivious)
    write_back_from_scratch_to_stash();

    // Update the ORAM Tree (Secure by ORAM design)
    write_back_from_stash_to_tree(old_leaf);

    // Clear the scratchpad
    scratchpad->leaf = 0;
    scratchpad->new_addr = 0;
    scratchpad->old_addr = 0;
  }

  // step1: stroll through pmap to find leaf
  ADDRTY new_addr = 0;
  ADDRTY offset = 0;
  ADDRTY closest_addr = 0;
  OTYPE type = __CODE;

  //byunggill
  int leaf = locate_addr_from_pmap(old_addr, &closest_addr,
        &new_addr, &offset, &type);
  type = cur;

  bool flag = false;
  if (leaf == -1) {
      // XXX. should come here only once
      CHECK(flag == false);
      update_pmap_count +=1;

      if (cur == __CODE) {
        sgx_print_string("code block does not exist: ");
        sgx_print_hex(old_addr);
        sgx_print_string("\n");
      } else {
        sgx_print_string("data block does not exist: ");
        sgx_print_hex(old_addr);
        sgx_print_string("\n");
      }

      flag = true;
      return old_addr;
  }

  // Debugging purposes
  num_oram_reads++;

  // Copy path from ORAM Tree to the stash (Secure by ORAM design)
  copy_path_onto_stash(leaf);

  // Find the block from the stash (Oblivious)
  find_req_block_from_stash(closest_addr, new_addr, leaf);

  oramly_translated++;

  /* Sanity Checks for code-based (added by BG) */
  if (type == __CODE) {
    CHECK(scratch.leaf > 0);
    if(*(scratch.buf + 61) != 0x41)
    {
      DOUT("61 error, %d\n", *(scratch.buf+61));
      assert(false);
    }
    if (*(scratch.buf + 62) != (char)0xff){
      DOUT("62 error, %d\n", *(scratch.buf + 62));
      assert(false);
    }
    if(*(scratch.buf + 63) != (char)0xe6)
    {
      DOUT("63 error, %d\n", *(scratch.buf + 62));
      assert(false);
    }
   }

  return (ADDRTY) (scratchpad->buf + offset);
}



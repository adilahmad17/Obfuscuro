#ifndef __ORAM_H__
#define __ORAM_H__

#include <stdint.h>

#define NUM_TREE_CODE_LEAF     256
#define NUM_TREE_CODE_NODES    ((2*NUM_TREE_CODE_LEAF) - 1) // nodes in the tree

#define NUM_TREE_DATA_LEAF     256
#define NUM_TREE_DATA_NODES    ((2*NUM_TREE_DATA_LEAF) - 1) // nodes in the tree

#define NUM_NODES_BLOCKS  1                       // block per nodes

#define NUM_STASH_CODE_BLOCKS     16//20    //blocks in the stash
#define NUM_STASH_DATA_BLOCKS     16//20    // blocks in the stash

#define STACK_FRAME_NUM   80

#define ORAM          1
#define REGISTERS     0
#define PAGE_SIZE     64 //Byunggill, I changed it from 64

// SEC == 0 -> Insecure
// SEC == 1 -> CMOV
// SEC == 2 -> Registers
#define SEC           1  // Byunggill 2 -> 1
                        // The SEC should be 1 for Oblivex evaluations

/* struct for each oram block */
typedef struct {
  char memory[PAGE_SIZE];
} tree_block_t;

/* struct for tree nodes */
typedef struct {
  tree_block_t blocks[NUM_NODES_BLOCKS];
} tree_node_t;

/* struct for position map */
typedef struct {
  int leaf;
  ADDRTY old_addr;
  ADDRTY new_addr;
  SIZETY size;
  OTYPE type;
} pmap_block_t;

/* struct for stash map */
typedef struct {
  int leaf;
  bool real;
  bool filled;
  ADDRTY new_addr;
  ADDRTY old_addr;
} smap_block_t;

/* struct for tree map */
typedef struct {
  int filled;
} tmap_block_t;

typedef struct {
  int leaf;
  ADDRTY old_addr;
  ADDRTY new_addr;
  char* buf;
} scratchbuf_t;

/* Data structures required for code-based ORAM */
extern tree_block_t ostash_code[NUM_STASH_CODE_BLOCKS];
extern tree_node_t otree_code[NUM_TREE_CODE_NODES];
extern smap_block_t osmap_code[NUM_STASH_CODE_BLOCKS];
extern pmap_block_t oposmap_code[NUM_TREE_CODE_LEAF];
extern tmap_block_t otmap_code[NUM_TREE_CODE_NODES];

/* Data structures required for data-based ORAM */
extern tree_block_t ostash_data[NUM_STASH_DATA_BLOCKS];
extern tree_node_t otree_data[NUM_TREE_DATA_NODES];
extern smap_block_t osmap_data[NUM_STASH_DATA_BLOCKS];
extern pmap_block_t oposmap_data[NUM_TREE_DATA_LEAF];
extern tmap_block_t otmap_data[NUM_TREE_DATA_NODES];

extern char dummy_memory[PAGE_SIZE];

/* scratch buffer */
extern scratchbuf_t scratch;
extern scratchbuf_t scratch_data;

#if defined(__cplusplus)
extern "C" {
#endif

/* from rerand_oram_init.cc */
void populate_oram(ADDRTY addr, size_t size, OTYPE type);
void populate_stack(ADDRTY start_addr);

/* from rerand_oram_ops.cc */
ADDRTY otranslate(ADDRTY old_address, OTYPE type);
ADDRTY scratch_translate(ADDRTY old_address, OTYPE type);

/* from rerand_oram_util.cc */
int get_rand32(unsigned int* rand);
int is_in_path(int leaf, int path_index);
void debug_stash_map();

/* from rerand_oram_cmov.cc */
uint64_t cmov(uint64_t val1, uint64_t test, uint64_t val2);
void cmov_memory(char* addr1, char* addr2, uint64_t size, uint32_t test);

/* from rerand_oram_reg.cc */
void reg_move_to_stash(uint64_t addr, int index);
void reg_copy_from_stash(uint64_t addr, int index);
void debug_stash(int index);

unsigned int __rerand_get_executed_code_blocks();
unsigned int __rerand_get_fetched_data_blocks();
#if defined(__cplusplus)
}
#endif

#endif

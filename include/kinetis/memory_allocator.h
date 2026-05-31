#ifndef MEMORY_ALLOCATOR_H
#define MEMORY_ALLOCATOR_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

// Type definitions for consistent integer sizes
#ifndef __KERNEL__
typedef uint32_t u32;
typedef uint64_t u64;
#endif

// Maximum number of supported memory regions
#define MAX_MEM_REGIONS 4

// Memory block status
typedef enum {
	BLOCK_FREE,
	BLOCK_USED
} block_status;

// Memory block control structure (public, for reference only)
typedef struct {
	void *start;          // Start address of memory block
	u32 size;           // Size of memory block
	block_status status;   // Status of memory block
} memory_block;

// Region configuration - defines one continuous RAM region
// Used at init time to describe all available RAM regions
typedef struct {
	void    *start;       // Region start address
	u32      size;        // Region size in bytes
	const char *name;     // Human-readable name (e.g. "SRAM1", "CCM")
} mem_region_config;

// =========================================================================
// Initialization API — RECOMMENDED: use mem_init_default()
// =========================================================================
//
// The allocator reads RAM region boundaries from LINKER SCRIPT SYMBOLS,
// NOT hardcoded addresses. This ensures portability between MCUs and
// stays in sync when the linker script changes.
//
// Pools are numbered _mempool0 through _mempool3 (defined in the linker
// script as .mempoolN sections). mem_init_default() iterates all 4 slots
// and registers only non-zero pools as allocatable regions.
//
// Backward compat: _memory_pool_start/end are PROVIDE aliases to _mempool0_*.
//
// Adding a new pool (e.g. external SDRAM):
//   1. In linker script, add:  .mempool2 (NOLOAD) : { ... } >NEW_MEMORY
//   2. Rebuild — C code auto-detects it. No source changes needed.
//
// Usage:
//   void main(void) {
//       mem_init_default();  // auto-discovers all pool sections
//       void *p = mem_alloc(256);
//       void *dma_buf = mem_alloc_from_region(0, 256);  // from POOL0 (DMA-safe)
//   }
//
// To customize regions (advanced), use mem_init_with_config():
//   mem_region_config cfgs[] = {
//       { (void *)&_mempool0_start, POOL0_SIZE, "SRAM-POOL" },
//       { (void *)&_mempool1_start, POOL1_SIZE, "CCM-POOL"  },
//   };
//   mem_init_with_config(cfgs, 2);
//
// =========================================================================

// Recommended: auto-detect all allocatable regions from linker script symbols
// Iterates _mempool0 … _mempool3, registers non-zero slots
void mem_init_default(void);

// Advanced: initialize with explicit region configuration table
void mem_init_with_config(const mem_region_config *configs, int num_regions);

// Backward compatible: single region from .memory_pool linker symbols
// Internally calls mem_init_default()
void mem_init(void);

// Allocation API (backward compatible - tries regions in order)
void *mem_alloc(u32 size);
void *mem_alloc_aligned(u32 size, u32 alignment);
void mem_free(void *ptr);
void *mem_calloc(u32 count, u32 size);
void *mem_calloc_aligned(u32 count, u32 size, u32 alignment);
void *mem_realloc(void *ptr, u32 new_size);
void *mem_realloc_aligned(void *ptr, u32 new_size, u32 alignment);

// New: allocate from a specific region by index
// Useful when memory must reside in a particular RAM region (e.g. DMA buffer in SRAM, not CCM)
void *mem_alloc_from_region(int region_id, u32 size);
void *mem_alloc_aligned_from_region(int region_id, u32 size, u32 alignment);

// Region info query
int  mem_get_region_count(void);
const char *mem_get_region_name(int region_id);
u32  mem_get_region_size(int region_id);
u32  mem_get_region_used(int region_id);
u32  mem_get_region_free(int region_id);

// Statistics functions (aggregated across all regions)
u32 mem_get_used(void);
u32 mem_get_free(void);
int mem_get_usage_percent(void);
int mem_get_used_blocks_count(void);
int mem_get_free_blocks_count(void);
u32 mem_get_largest_free_block(void);
int mem_get_fragmentation_percent(void);

// Display functions
void mem_print_stats(void);
void mem_print_summary(void);

// Utility functions
int mem_is_aligned(void *ptr, u32 alignment);
void mem_print_alignment_info(void *ptr);

// Helper functions
void mem_format_size(u32 size, char *buffer, int buffer_size);

void test_memory_allocator(void);

#endif // MEMORY_ALLOCATOR_H

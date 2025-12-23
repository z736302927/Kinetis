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

// Memory block status
typedef enum {
	BLOCK_FREE,
	BLOCK_USED
} block_status;

// Memory block control structure
typedef struct {
	void *start;          // Start address of memory block
	u32 size;           // Size of memory block
	block_status status;   // Status of memory block
} memory_block;

// Public API functions
void mem_init(void);
void *mem_alloc(u32 size);
void *mem_alloc_aligned(u32 size, u32 alignment);
void mem_free(void* ptr);
void *mem_calloc(u32 count, u32 size);
void *mem_calloc_aligned(u32 count, u32 size, u32 alignment);
void *mem_realloc(void* ptr, u32 new_size);
void *mem_realloc_aligned(void* ptr, u32 new_size, u32 alignment);

// Statistics functions
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
void mem_print_human_readable_info(void);

// Utility functions
int mem_is_aligned(void* ptr, u32 alignment);
void mem_print_alignment_info(void* ptr);

// Helper functions
void mem_format_size(u32 size, char* buffer, int buffer_size);

void test_memory_allocator(void);

#endif // MEMORY_ALLOCATOR_H
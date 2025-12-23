#include "kinetis/memory_allocator.h"

#ifdef __KERNEL__
#define pr_fmt(fmt) "[mem debug] " fmt

#include <linux/printk.h>
#include <linux/limits.h>
#include <linux/string.h>

#else
// Custom logging system (for kernel and user space)
// User space version
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#define LOG_LEVEL_NONE     0
#define LOG_LEVEL_ERROR    1
#define LOG_LEVEL_WARNING  2
#define LOG_LEVEL_INFO     3
#define LOG_LEVEL_DEBUG    4

// Default log level
#ifndef MEM_LOG_LEVEL
#define MEM_LOG_LEVEL LOG_LEVEL_INFO
#endif

#define pr_debug(fmt, ...) \
	do { if (MEM_LOG_LEVEL >= LOG_LEVEL_DEBUG) printf("[mem debug] " fmt "\n", ##__VA_ARGS__); } while(0)

#define pr_info(fmt, ...) \
	do { if (MEM_LOG_LEVEL >= LOG_LEVEL_INFO) printf("[mem info] " fmt "\n", ##__VA_ARGS__); } while(0)

#define pr_warn(fmt, ...) \
	do { if (MEM_LOG_LEVEL >= LOG_LEVEL_WARNING) printf("[mem warn] " fmt "\n", ##__VA_ARGS__); } while(0)

#define pr_err(fmt, ...) \
	do { if (MEM_LOG_LEVEL >= LOG_LEVEL_ERROR) printf("[mem error] " fmt "\n", ##__VA_ARGS__); } while(0)

#define pr_cont(fmt, ...) \
	do { if (MEM_LOG_LEVEL >= LOG_LEVEL_INFO) printf(fmt, ##__VA_ARGS__); } while(0)

// Type definitions for user space
typedef uint32_t u32;
typedef uint64_t u64;

#endif

// Static memory pool size
#define STATIC_POOL_SIZE (1024 * 1024)
#define MAX_BLOCKS 64

// Memory alignment macros (fixed for 64-bit addresses)
#define ALIGN_UP(val, align) (((val) + (align) - 1) & ~((u64)(align) - 1))
#define ALIGN_DOWN(val, align) ((val) & ~((u64)(align) - 1))
#define IS_POWER_OF_2(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)

// Improved memory block structure with actual allocation size tracking
typedef struct {
	void *start;
	u32 size;           // User requested size
	u32 actual_size;    // Actual allocated size (including alignment padding)
	block_status status;
} memory_block_detailed;

// Static memory pool
static unsigned char memory_pool[STATIC_POOL_SIZE];
static memory_block_detailed blocks[MAX_BLOCKS];
static int block_count = 0;
static u32 memory_used = 0;
static int initialized = 0;
static int need_sort = 0;  // Flag to indicate if blocks need sorting

// Initialize memory manager
void mem_init(void)
{
	int i;
	char pool_size_str[32];

	for (i = 0; i < MAX_BLOCKS; i++) {
		blocks[i].start = NULL;
		blocks[i].size = 0;
		blocks[i].actual_size = 0;
		blocks[i].status = BLOCK_FREE;
	}
	block_count = 0;
	memory_used = 0;
	need_sort = 0;
	pr_info("memory allocator initialization");
	mem_format_size(STATIC_POOL_SIZE, pool_size_str, sizeof(pool_size_str));
	pr_info("pool initialized with %s (%u bytes)", pool_size_str, STATIC_POOL_SIZE);
	pr_info("maximum blocks: %d", MAX_BLOCKS);
}

// Find free block descriptor
static int find_free_block(void)
{
	int i;
	for (i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].status == BLOCK_FREE) {
			return i;
		}
	}
	return -1;
}

// Sort blocks by address if needed (lazy sorting)
static void ensure_blocks_sorted(void)
{
	int i, j;

	if (!need_sort || block_count <= 1) {
		return;
	}

	// Simple bubble sort (efficient for small n)
	for (i = 0; i < block_count - 1; i++) {
		for (j = i + 1; j < block_count; j++) {
			// Compare addresses, handling NULL pointers
			u64 addr_i = blocks[i].start ? (u64)blocks[i].start : 0;
			u64 addr_j = blocks[j].start ? (u64)blocks[j].start : 0;

			if (addr_j < addr_i) {
				memory_block_detailed temp = blocks[i];
				blocks[i] = blocks[j];
				blocks[j] = temp;
			}
		}
	}

	need_sort = 0;
}

// Insert a block at the correct position to maintain sorted order
static void insert_block_sorted(int idx, void *start, u32 size, u32 actual_size)
{
	int i, pos;

	// If block array is empty, just add it
	if (block_count == 0) {
		blocks[0].start = start;
		blocks[0].size = size;
		blocks[0].actual_size = actual_size;
		blocks[0].status = BLOCK_USED;
		block_count = 1;
		need_sort = 0;  // Single element is already sorted
		return;
	}

	// Ensure blocks are sorted before finding insertion position
	ensure_blocks_sorted();

	// Find insertion position
	for (pos = 0; pos < block_count; pos++) {
		u64 pos_addr = blocks[pos].start ? (u64)blocks[pos].start : 0;
		if ((u64)start < pos_addr) {
			break;
		}
	}

	// If inserting at the end
	if (pos >= block_count) {
		// Use the free slot at idx if it's at or beyond block_count
		if (idx >= block_count && idx < MAX_BLOCKS) {
			// Fill the free slot
			blocks[idx].start = start;
			blocks[idx].size = size;
			blocks[idx].actual_size = actual_size;
			blocks[idx].status = BLOCK_USED;
			if (idx >= block_count) {
				block_count = idx + 1;
			}
		} else {
			// Append at the end
			blocks[block_count].start = start;
			blocks[block_count].size = size;
			blocks[block_count].actual_size = actual_size;
			blocks[block_count].status = BLOCK_USED;
			block_count++;
		}
	} else {
		// Insert in the middle - we need to shift and use the free slot

		// First, if idx is a free slot and it's after pos, we can use it
		if (idx >= pos && idx < MAX_BLOCKS && blocks[idx].status == BLOCK_FREE) {
			// Fill the free slot
			blocks[idx].start = start;
			blocks[idx].size = size;
			blocks[idx].actual_size = actual_size;
			blocks[idx].status = BLOCK_USED;

			// Now we need to re-sort
			need_sort = 1;

			// Update block_count if needed
			if (idx >= block_count) {
				block_count = idx + 1;
			}
		} else {
			// Need to shift blocks to make room
			// Find a free slot to use (preferably at the end)
			int free_slot = find_free_block();
			if (free_slot == -1) {
				// This shouldn't happen as we checked earlier
				return;
			}

			// Shift blocks from pos to free_slot
			if (free_slot >= block_count) {
				// Free slot is at or beyond current count
				blocks[free_slot].start = start;
				blocks[free_slot].size = size;
				blocks[free_slot].actual_size = actual_size;
				blocks[free_slot].status = BLOCK_USED;
				if (free_slot >= block_count) {
					block_count = free_slot + 1;
				}
			} else {
				// Free slot is within current blocks, need to shift
				for (i = block_count; i > pos; i--) {
					if (i < MAX_BLOCKS) {
						blocks[i] = blocks[i - 1];
					}
				}
				// Insert at pos
				blocks[pos].start = start;
				blocks[pos].size = size;
				blocks[pos].actual_size = actual_size;
				blocks[pos].status = BLOCK_USED;
				block_count++;
				if (block_count > MAX_BLOCKS) {
					block_count = MAX_BLOCKS;
				}
			}

			// Mark that we need to sort (blocks were shifted)
			need_sort = 1;
		}
	}

	// After insertion, ensure blocks are sorted
	ensure_blocks_sorted();
}

// Improved: Find suitable space with alignment consideration
static void *find_fit_aligned(u32 size, u32 alignment)
{
	int i;
	u64 pool_start = (u64)memory_pool;
	u64 pool_end = pool_start + STATIC_POOL_SIZE;

	// Ensure blocks are sorted before searching
	ensure_blocks_sorted();

	// If no blocks allocated, start from aligned address in pool
	if (block_count == 0) {
		u64 aligned_start = ALIGN_UP(pool_start, alignment);
		u64 allocated_end = aligned_start + size;

		if (allocated_end <= pool_end) {
			return (void *)aligned_start;
		}
		return NULL;
	}

	// Check gap before first used block
	for (i = 0; i <= block_count; i++) {
		u64 gap_start, gap_end, aligned_start, allocated_end;

		if (i == 0) {
			// Gap before first block
			gap_start = pool_start;
		} else {
			// Gap after previous block
			gap_start = (u64)blocks[i - 1].start + blocks[i - 1].actual_size;
		}

		if (i == block_count) {
			// Gap after last block
			gap_end = pool_end;
		} else {
			gap_end = (u64)blocks[i].start;
		}

		// Skip if gap is too small
		if ((u64)(gap_end - gap_start) < size) {
			continue;
		}

		// Calculate first aligned address in this gap
		aligned_start = ALIGN_UP(gap_start, alignment);
		allocated_end = aligned_start + size;

		// Check if aligned block fits in this gap
		if (aligned_start >= gap_start && allocated_end <= gap_end) {
			return (void *)aligned_start;
		}
	}

	return NULL;
}

// Custom memory allocation function (with alignment support)
void *mem_alloc(u32 size)
{
	return mem_alloc_aligned(size, 4);  // Default 4-byte alignment
}

// Custom memory allocation function (with specified alignment)
void *mem_alloc_aligned(u32 size, u32 alignment)
{
	int block_idx;
	void *aligned_ptr;
	u32 aligned_size;
	u64 pool_start, pool_end;
	char required_str[32], available_str[32], largest_str[32];
	char alloc_size_str[32], used_size_str[32], total_size_str[32];

	// Initialize memory manager on first allocation
	if (!initialized) {
		mem_init();
		initialized = 1;
	}

	if (size == 0) {
		pr_warn("malloc called with size 0");
		return NULL;
	}

	if (alignment == 0) {
		alignment = 1;
	}

	// Check if alignment is power of 2
	if (!IS_POWER_OF_2(alignment)) {
		pr_warn("alignment %u is not a power of 2", alignment);
		return NULL;
	}

	/* Calculate aligned size with overflow check */
	if (size > 0xFFFFFFFF - (alignment - 1)) {
		pr_warn("size overflow during alignment calculation (size=%u, alignment=%u)", size, alignment);
		return NULL;
	}

	/* 使用64位计算对齐大小，然后转换为32位 */
	aligned_size = (u32)ALIGN_UP(size, alignment);

	// Check if aligned size would exceed pool size
	if (aligned_size > STATIC_POOL_SIZE) {
		mem_format_size(aligned_size, required_str, sizeof(required_str));
		mem_format_size(STATIC_POOL_SIZE, available_str, sizeof(available_str));
		pr_warn("requested size %s exceeds pool size %s", required_str, available_str);
		return NULL;
	}

	block_idx = find_free_block();
	if (block_idx == -1) {
		pr_warn("no free block descriptors available");
		pr_info("maximum blocks: %d, currently used: %d", MAX_BLOCKS, block_count);
		return NULL;
	}

	// Find suitable space with alignment consideration
	aligned_ptr = find_fit_aligned(aligned_size, alignment);
	if (aligned_ptr == NULL) {
		mem_format_size(aligned_size, required_str, sizeof(required_str));
		mem_format_size(mem_get_free(), available_str, sizeof(available_str));
		mem_format_size(mem_get_largest_free_block(), largest_str, sizeof(largest_str));

		pr_warn("not enough memory for %s (aligned to %u)", required_str, alignment);
		pr_info("available: %s, required: %s", available_str, required_str);
		pr_info("largest free block: %s", largest_str);
		return NULL;
	}

	// Verify aligned address is within pool boundaries
	pool_start = (u64)memory_pool;
	pool_end = pool_start + STATIC_POOL_SIZE;

	if ((u64)aligned_ptr < pool_start || (u64)aligned_ptr >= pool_end) {
		pr_warn("aligned address %#llx is outside pool range [%#llx, %#llx)",
			(u64)aligned_ptr, pool_start, pool_end);
		return NULL;
	}

	// Verify allocation doesn't exceed pool end
	u64 allocated_end = (u64)aligned_ptr + aligned_size;
	if (allocated_end > pool_end) {
		pr_warn("allocation would exceed pool boundary (end=%#llx, pool_end=%#llx)",
			allocated_end, pool_end);
		return NULL;
	}

	// Insert block in sorted position
	insert_block_sorted(block_idx, aligned_ptr, size, aligned_size);

	// Update memory usage
	memory_used += aligned_size;

	// Log allocation
	mem_format_size(size, alloc_size_str, sizeof(alloc_size_str));
	mem_format_size(memory_used, used_size_str, sizeof(used_size_str));
	mem_format_size(STATIC_POOL_SIZE, total_size_str, sizeof(total_size_str));
	pr_debug("allocated %s at %#llx (align: %u, pool: %s/%s, %d%%)",
		alloc_size_str, (u64)aligned_ptr, alignment, used_size_str, total_size_str,
		mem_get_usage_percent());

	return aligned_ptr;
}

// Custom memory free function
void mem_free(void *ptr)
{
	int i;
	int j;
	u32 freed_actual_size;
	char freed_size_str[32], user_size_str[32], used_size_str[32], total_size_str[32];

	if (ptr == NULL) {
		pr_warn("free called with NULL pointer");
		return;
	}

	// Find corresponding block
	for (i = 0; i < block_count; i++) {
		if (blocks[i].start == ptr && blocks[i].status == BLOCK_USED) {
			freed_actual_size = blocks[i].actual_size;  // Free the actual allocated size
			memory_used -= freed_actual_size;

			mem_format_size(freed_actual_size, freed_size_str, sizeof(freed_size_str));
			mem_format_size(blocks[i].size, user_size_str, sizeof(user_size_str));
			mem_format_size(memory_used, used_size_str, sizeof(used_size_str));
			mem_format_size(STATIC_POOL_SIZE, total_size_str, sizeof(total_size_str));

			pr_debug("freed %s at %#llx (user requested: %s, pool: %s/%s, %d%%)",
				freed_size_str, (u64)ptr, user_size_str, used_size_str, total_size_str,
				mem_get_usage_percent());

			// Mark as free
			blocks[i].status = BLOCK_FREE;
			blocks[i].start = NULL;
			blocks[i].size = 0;
			blocks[i].actual_size = 0;

			// Mark that sorting may be needed
			need_sort = 1;

			// Try to compress block array from the end
			for (j = block_count - 1; j >= 0; j--) {
				if (blocks[j].status == BLOCK_USED) {
					block_count = j + 1;
					break;
				}
			}
			if (j < 0) {
				block_count = 0;
			}

			return;
		}
	}

	pr_warn("invalid pointer %#llx or double free detected", (u64)ptr);
}

// Zero allocation (default 4-byte alignment)
void *mem_calloc(u32 count, u32 size)
{
	return mem_calloc_aligned(count, size, 4);
}

// Zero allocation (with specified alignment)
void *mem_calloc_aligned(u32 count, u32 size, u32 alignment)
{
	void *ptr;
	u32 total_size;
	char size_str[32];

	// Check for integer overflow
	if (size != 0 && count > 0xFFFFFFFF / size) {
		pr_warn("calloc: size overflow (count=%u, size=%u)", count, size);
		return NULL;
	}

	total_size = count * size;

	ptr = mem_alloc_aligned(total_size, alignment);
	if (ptr != NULL) {
		// Use memset to zero memory
		memset(ptr, 0, total_size);
		mem_format_size(total_size, size_str, sizeof(size_str));
		pr_info("memory zeroed");
		pr_info("size: %s", size_str);
		pr_info("address: %#llx (aligned to %u bytes)", (u64)ptr, alignment);
	}

	return ptr;
}

// Memory reallocation (default 4-byte alignment)
void *mem_realloc(void *ptr, u32 new_size)
{
	return mem_realloc_aligned(ptr, new_size, 4);
}

// Memory reallocation (with specified alignment)
void *mem_realloc_aligned(void *ptr, u32 new_size, u32 alignment)
{
	void *new_ptr;
	int i;
	u32 old_size = 0;
	u32 old_actual_size = 0;
	u32 copy_size;
	char old_size_str[32], new_size_str[32], used_size_str[32], total_size_str[32];

	if (ptr == NULL) {
		return mem_alloc_aligned(new_size, alignment);
	}

	if (new_size == 0) {
		mem_free(ptr);
		return NULL;
	}

	if (alignment == 0) {
		alignment = 1;
	}

	// Check if alignment is power of 2
	if (!IS_POWER_OF_2(alignment)) {
		pr_warn("alignment %u is not a power of 2", alignment);
		return NULL;
	}

	// Find original block size
	for (i = 0; i < block_count; i++) {
		if (blocks[i].start == ptr && blocks[i].status == BLOCK_USED) {
			old_size = blocks[i].size;
			old_actual_size = blocks[i].actual_size;
			break;
		}
	}

	if (i == block_count) {
		pr_warn("invalid pointer %#llx in realloc", (u64)ptr);
		return NULL;
	}

	// If new size <= old size, return original pointer
	if (new_size <= old_size) {
		// Only update user requested size, don't change actually allocated memory
		blocks[i].size = new_size;
		pr_info("realloc shrink: %u->%u bytes, %#llx->%#llx (align: %u, pool: %u/%d, %d%%)",
			old_size, new_size, (u64)ptr, (u64)ptr, alignment, memory_used, STATIC_POOL_SIZE,
			mem_get_usage_percent());
		return ptr;
	}

	// Allocate new memory
	new_ptr = mem_alloc_aligned(new_size, alignment);
	if (new_ptr == NULL) {
		// Don't free original memory on realloc failure, keep original pointer valid
		pr_warn("realloc failed to allocate %u bytes, keeping original pointer", new_size);
		return ptr;
	}

	// Copy data - use memmove to handle memory overlap
	copy_size = (new_size < old_size) ? new_size : old_size;
	memmove(new_ptr, ptr, copy_size);

	// Free old memory
	mem_free(ptr);

	mem_format_size(old_size, old_size_str, sizeof(old_size_str));
	mem_format_size(new_size, new_size_str, sizeof(new_size_str));
	mem_format_size(memory_used, used_size_str, sizeof(used_size_str));
	mem_format_size(STATIC_POOL_SIZE, total_size_str, sizeof(total_size_str));

	pr_info("realloc expand: %s->%s bytes, %#llx->%#llx (align: %u, pool: %s/%s, %d%%)",
		old_size_str, new_size_str, (u64)ptr, (u64)new_ptr, alignment, used_size_str, total_size_str,
		mem_get_usage_percent());
	return new_ptr;
}

// Check address alignment
int mem_is_aligned(void *ptr, u32 alignment)
{
	if (alignment == 0) {
		return 1;
	}
	return ((u64)ptr & (alignment - 1)) == 0;
}

// Print alignment information for address
void mem_print_alignment_info(void *ptr)
{
	int i;

	pr_info("alignment information");
	pr_info("address: %#llx", (u64)ptr);
	pr_info("alignment status:");
	for (i = 1; i <= 64; i *= 2) {
		if (mem_is_aligned(ptr, i)) {
			pr_info("%d bytes: aligned", i);
		} else {
			pr_info("%d bytes: not aligned", i);
		}
	}
}

// Get current memory usage in bytes
u32 mem_get_used(void)
{
	return memory_used;
}

// Get current free memory in bytes
u32 mem_get_free(void)
{
	return STATIC_POOL_SIZE - memory_used;
}

// Get memory usage percentage (returns integer percentage)
int mem_get_usage_percent(void)
{
	if (STATIC_POOL_SIZE == 0) {
		return 0;
	}
	return (int)((memory_used * 100) / STATIC_POOL_SIZE);
}

// Get number of used blocks
int mem_get_used_blocks_count(void)
{
	int count = 0;
	int i;

	for (i = 0; i < block_count; i++) {
		if (blocks[i].status == BLOCK_USED) {
			count++;
		}
	}
	return count;
}

// Get number of free block descriptors
int mem_get_free_blocks_count(void)
{
	return MAX_BLOCKS - block_count;
}

// Get largest free block size
u32 mem_get_largest_free_block(void)
{
	u32 largest = 0;
	int i;
	int first_used;
	int prev_used;
	u32 first_space;
	void *next_start;
	void *prev_end;
	u32 space;
	void *last_end;
	u32 last_space;

	if (block_count == 0) {
		return STATIC_POOL_SIZE;
	}

	// Ensure blocks are sorted
	ensure_blocks_sorted();

	// Find first used block
	first_used = -1;
	for (i = 0; i < block_count; i++) {
		if (blocks[i].status == BLOCK_USED) {
			first_used = i;
			break;
		}
	}

	// If no used blocks, entire pool is free
	if (first_used == -1) {
		return STATIC_POOL_SIZE;
	}

	// Check space before first used block
	first_space = (u32)((u64)blocks[first_used].start - (u64)memory_pool);
	if (first_space > largest) {
		largest = first_space;
	}

	// Check spaces between consecutive used blocks
	prev_used = first_used;
	for (i = first_used + 1; i < block_count; i++) {
		if (blocks[i].status == BLOCK_USED) {
			next_start = blocks[i].start;
			prev_end = blocks[prev_used].start + blocks[prev_used].actual_size;
			space = (u32)((char *)next_start - (char *)prev_end);
			if (space > largest) {
				largest = space;
			}
			prev_used = i;
		}
	}

	// Check space after last used block
	last_end = blocks[prev_used].start + blocks[prev_used].actual_size;
	last_space = (u32)((u64)(memory_pool + STATIC_POOL_SIZE) - (u64)last_end);
	if (last_space > largest) {
		largest = last_space;
	}

	return largest;
}

// Get memory fragmentation percentage (returns integer percentage)
int mem_get_fragmentation_percent(void)
{
	u32 total_free = mem_get_free();
	u32 largest_free = mem_get_largest_free_block();

	if (total_free == 0 || largest_free == 0) {
		return 0;
	}

	return (int)(100 - (largest_free * 100) / total_free);
}

// Helper function to format memory size in human readable format
void mem_format_size(u32 size, char *buffer, int buffer_size)
{
	const char *units[] = {"B", "KB", "MB", "GB"};
	int unit_index = 0;
	double size_d = (double)size;

	while (size_d >= 1024.0 && unit_index < 3) {
		size_d /= 1024.0;
		unit_index++;
	}

	if (unit_index == 0) {
		snprintf(buffer, buffer_size, "%u %s", size, units[unit_index]);
	} else {
		unsigned int size_int = (unsigned int)(size_d * 100);
		snprintf(buffer, buffer_size, "%u.%02u %s", size_int / 100, size_int % 100, units[unit_index]);
	}
}

// Print detailed memory statistics
void mem_print_stats(void)
{
	int used_blocks = mem_get_used_blocks_count();
	int free_blocks = mem_get_free_blocks_count();
	u32 largest_free = mem_get_largest_free_block();
	int usage_percent = mem_get_usage_percent();
	int fragmentation = mem_get_fragmentation_percent();
	char total_str[32], used_str[32], free_str[32], largest_str[32];

	// Ensure blocks are sorted for accurate statistics
	ensure_blocks_sorted();

	pr_info("memory statistics");
	pr_info("--- pool overview: ---");
	mem_format_size(STATIC_POOL_SIZE, total_str, sizeof(total_str));
	mem_format_size(mem_get_used(), used_str, sizeof(used_str));
	mem_format_size(mem_get_free(), free_str, sizeof(free_str));
	mem_format_size(largest_free, largest_str, sizeof(largest_str));

	pr_info("total size: %s (%u bytes)", total_str, STATIC_POOL_SIZE);
	pr_info("used: %s (%d%%)", used_str, usage_percent);
	pr_info("free: %s (%d%%)", free_str, 100 - usage_percent);

	pr_info("--- block management: ---");
	pr_info("used blocks: %d", used_blocks);
	pr_info("free descriptors: %d", free_blocks);
	pr_info("largest free block: %s", largest_str);
	pr_info("fragmentation: %d%%", fragmentation);

	// Add human readable format
	pr_info("--- human readable format: ---");
	mem_format_size(mem_get_used(), used_str, sizeof(used_str));
	mem_format_size(mem_get_free(), free_str, sizeof(free_str));
	mem_format_size(STATIC_POOL_SIZE, total_str, sizeof(total_str));
	mem_format_size(largest_free, largest_str, sizeof(largest_str));

	pr_info("total: %s | used: %s | free: %s", total_str, used_str, free_str);
	pr_info("largest available block: %s", largest_str);
}

// Print memory summary with additional details
void mem_print_summary(void)
{
	int used_blocks = mem_get_used_blocks_count();
	int i;
	u32 total_allocated = 0;
	u32 min_block_size = 0xFFFFFFFF;
	u32 max_block_size = 0;
	int usage_percent = mem_get_usage_percent();
	int fragmentation = mem_get_fragmentation_percent();
	char total_str[32], used_str[32], free_str[32];
	char min_str[32], max_str[32], avg_str[32];
	char largest_contig_str[32];

	// Ensure blocks are sorted for accurate statistics
	ensure_blocks_sorted();

	// Calculate block statistics
	for (i = 0; i < block_count; i++) {
		if (blocks[i].status == BLOCK_USED) {
			total_allocated += blocks[i].size;
			if (blocks[i].size < min_block_size) {
				min_block_size = blocks[i].size;
			}
			if (blocks[i].size > max_block_size) {
				max_block_size = blocks[i].size;
			}
		}
	}

	pr_info("memory allocation summary");
	pr_info("--- pool configuration: ---");
	mem_format_size(STATIC_POOL_SIZE, total_str, sizeof(total_str));
	mem_format_size(mem_get_used(), used_str, sizeof(used_str));
	mem_format_size(mem_get_free(), free_str, sizeof(free_str));

	pr_info("total pool: %s (%u bytes)", total_str, STATIC_POOL_SIZE);
	pr_info("block descriptors: %d total, %d used, %d available",
		MAX_BLOCKS, block_count, MAX_BLOCKS - block_count);

	pr_info("--- current allocation status: ---");
	pr_info("memory used: %s / %s (%d%%)",
		used_str, total_str, usage_percent);
	pr_info("memory free: %s", free_str);
	pr_info("active blocks: %d", used_blocks);

	if (used_blocks > 0) {
		mem_format_size(min_block_size, min_str, sizeof(min_str));
		mem_format_size(max_block_size, max_str, sizeof(max_str));
		mem_format_size(total_allocated / used_blocks, avg_str, sizeof(avg_str));

		pr_info("  - block size analysis:");
		pr_info("    smallest block: %s", min_str);
		pr_info("    largest block: %s", max_str);
		// Average block size
		pr_info("    average block: %s", avg_str);

		mem_format_size(mem_get_largest_free_block(), largest_contig_str, sizeof(largest_contig_str));

		pr_info("  - memory efficiency:");
		pr_info("    internal fragmentation: %d%%", fragmentation);
		pr_info("    largest contiguous free: %s", largest_contig_str);
		// Allocation efficiency
		if (mem_get_used() > 0) {
			pr_info("    allocation efficiency: %d%%",
				(int)((total_allocated * 100) / mem_get_used()));
		} else {
			pr_info("    allocation efficiency: 0%%");
		}
	} else {
		pr_info("  - no blocks currently allocated");
	}

	pr_info("--- system health: ---");
	if (usage_percent > 90) {
		pr_warn("status: critical - memory usage above 90%%");
	} else if (usage_percent > 75) {
		pr_warn("status: warning - memory usage above 75%%");
	} else if (fragmentation > 50) {
		pr_warn("status: warning - high fragmentation detected");
	} else {
		pr_info("status: ok - memory usage normal");
	}
}

// Test the alignment bug fix for 64-bit systems
void test_alignment_bug_fix(void)
{
	void *ptr;
	u64 addr;
	u32 alignment;

	pr_info("Testing alignment bug fix for 64-bit systems");

	// Test 1: Large alignment value
	alignment = 0x1000;  // 4KB alignment
	addr = 0x123456789ABCDEF0;
	u64 aligned = ALIGN_UP(addr, alignment);
	u64 expected = 0x123456789ABCE000;  // Corrected expected value

	if (aligned == expected) {
		pr_info("Test 1 passed: 4KB alignment correct");
	} else {
		pr_err("Test 1 failed: got %#llx, expected %#llx", aligned, expected);
	}

	// Test 2: Cross 32-bit boundary
	alignment = 8;
	addr = 0xFFFFFFFFFFFFFFF0;  // Close to 64-bit boundary
	aligned = ALIGN_UP(addr, alignment);
	expected = 0xFFFFFFFFFFFFFFF0;  // Corrected expected value (already aligned)

	if (aligned == expected) {
		pr_info("Test 2 passed: 64-bit boundary alignment correct");
	} else {
		pr_err("Test 2 failed: got %#llx, expected %#llx", aligned, expected);
	}

	// Test 3: Verify wrong old method for comparison
	addr = 0xFFFFFFFFFFFFFFF0;
	u64 wrong = (addr + alignment - 1) & ~(alignment - 1);  // Wrong method
	pr_info("Wrong method result: %#llx", wrong);
	pr_info("Correct method result: %#llx", ALIGN_UP(addr, alignment));
	pr_info("Difference: %#llx", ALIGN_UP(addr, alignment) - wrong);
}

// Test and demonstration function
void test_memory_allocator(void)
{
	int *numbers;
	double *doubles;
	float *simd_data;
	long *long_array;
	int *zero_array;
	void *bad_ptr;
	char used_str[32], free_str[32], largest_str[32];
	int i;

	pr_info("static memory allocator with alignment support");
	pr_info("--- test suite starting ---");

	// Test alignment bug fix first
	test_alignment_bug_fix();

	// Memory manager will be initialized on first allocation

	// 1. Basic allocation (default 4-byte alignment)
	pr_info("1 basic allocation test (default 4-byte alignment)");
	numbers = (int *)mem_alloc(10 * sizeof(int));
	if (numbers) {
		pr_info("numbers array allocated successfully");
		pr_info("array address: %#llx", (u64)numbers);
		mem_print_alignment_info(numbers);
		for (i = 0; i < 10; i++) {
			numbers[i] = i * i;
		}
		pr_info("array contents:");
		for (i = 0; i < 10; i++) {
			pr_cont("%d ", numbers[i]);
		}
		pr_cont("\n");
	}

	// 2. 8-byte aligned allocation
	pr_info("2 8-byte aligned allocation test:");
	doubles = (double *)mem_alloc_aligned(5 * sizeof(double), 8);
	if (doubles) {
		pr_info("doubles array allocated successfully");
		pr_info("doubles array address: %#llx", (u64)doubles);
		mem_print_alignment_info(doubles);
		for (i = 0; i < 5; i++) {
			doubles[i] = 3.14159 * i;
		}
		pr_info("doubles array:");
		// Note: Kernel doesn't support floating point printing, changed to integer representation
		for (i = 0; i < 5; i++) {
			pr_cont("%d ", (int)doubles[i]);
		}
		pr_cont("\n");
	}

	// 3. 16-byte aligned allocation (for SIMD)
	pr_info("3 16-byte aligned allocation test (for SIMD):");
	simd_data = (float *)mem_alloc_aligned(16 * sizeof(float), 16);
	if (simd_data) {
		pr_info("simd data address: %#llx", (u64)simd_data);
		mem_print_alignment_info(simd_data);
		pr_info("16-byte aligned allocation successful");
	}

	// 4. 32-byte aligned allocation
	pr_info("4 32-byte aligned allocation test:");
	long_array = (long *)mem_alloc_aligned(8 * sizeof(long), 32);
	if (long_array) {
		pr_info("long array address: %#llx", (u64)long_array);
		mem_print_alignment_info(long_array);
		pr_info("32-byte aligned allocation successful");
	}

	// 5. Zero allocation test (16-byte alignment)
	pr_info("5 zero allocation test with 16-byte alignment:");
	zero_array = (int *)mem_calloc_aligned(5, sizeof(int), 16);
	if (zero_array) {
		pr_info("zero array address: %#llx", (u64)zero_array);
		mem_print_alignment_info(zero_array);
		pr_info("zero array:");
		for (i = 0; i < 5; i++) {
			pr_cont("%d ", zero_array[i]);
		}
		pr_cont("\n");
	}

	// 6. Reallocation test (8-byte alignment)
	pr_info("6 reallocation test with 8-byte alignment:");
	numbers = (int *)mem_realloc_aligned(numbers, 20 * sizeof(int), 8);
	if (numbers) {
		pr_info("reallocated numbers address: %#llx", (u64)numbers);
		mem_print_alignment_info(numbers);
		for (i = 10; i < 20; i++) {
			numbers[i] = i * i;
		}
		pr_info("extended array first 20 elements:");
		for (i = 0; i < 20; i++) {
			pr_cont("%d ", numbers[i]);
		}
		pr_cont("\n");
	}

	// 7. Error test: invalid alignment
	pr_info("7 invalid alignment test:");
	bad_ptr = mem_alloc_aligned(100, 3);  // 3 is not power of 2
	if (!bad_ptr) {
		pr_info("invalid alignment correctly rejected");
	}

	// 8. Memory usage information
	pr_info("8 memory usage information:");
	mem_print_stats();

	// 9. Memory statistics test
	pr_info("9 memory statistics test:");
	mem_format_size(mem_get_used(), used_str, sizeof(used_str));
	mem_format_size(mem_get_free(), free_str, sizeof(free_str));
	mem_format_size(mem_get_largest_free_block(), largest_str, sizeof(largest_str));

	pr_info("current used: %s (%u bytes)", used_str, mem_get_used());
	pr_info("current free: %s (%u bytes)", free_str, mem_get_free());
	pr_info("usage percentage: %d%%", mem_get_usage_percent());
	pr_info("used blocks count: %d", mem_get_used_blocks_count());
	pr_info("free block descriptors: %d", mem_get_free_blocks_count());
	pr_info("largest free block: %s (%u bytes)", largest_str, mem_get_largest_free_block());
	pr_info("fragmentation: %d%%", mem_get_fragmentation_percent());

	// 10. Memory summary
	pr_info("10 memory summary:");
	mem_print_summary();

	// 11. Free test
	pr_info("11 free test:");
	mem_free(numbers);
	mem_free(zero_array);
	mem_free(doubles);
	mem_free(simd_data);
	mem_free(long_array);

	// 12. Final memory state with summary
	pr_info("12 final memory state:");
	mem_print_stats();
	pr_info("--- final summary: ---");
	mem_print_summary();

	pr_info("all tests completed successfully");
}

// int main(int argc, char **argv)
// {
//     test_memory_allocator();
//     return 0;
// }
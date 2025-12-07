//#ifndef _FAKE__MCU_TYPES_H
//#define _FAKE__MCU_TYPES_H
//
//typedef char s8;
//typedef unsigned char u8;
//
//typedef short s16;
//typedef unsigned short u16;
//
//typedef int s32;
//typedef unsigned int u32;
//
//typedef long long s64;
//typedef unsigned long long u64;
//
//typedef s64	ktime_t;
//
//#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
//typedef u64 dma_addr_t;
//#else
//typedef u32 dma_addr_t;
//#endif
//
//typedef unsigned int gfp_t;
//typedef unsigned int slab_flags_t;
//typedef unsigned int fmode_t;
//
//#ifdef CONFIG_PHYS_ADDR_T_64BIT
//typedef u64 phys_addr_t;
//#else
//typedef u32 phys_addr_t;
//#endif
//
///*
// * This type is the placeholder for a hardware interrupt number. It has to be
// * big enough to enclose whatever representation is used by a given platform.
// */
//typedef unsigned long irq_hw_number_t;
//
//typedef struct {
//	int counter;
//} atomic_t;
//
//#define ATOMIC_INIT(i) { (i) }
//
//#ifdef CONFIG_64BIT
//typedef struct {
//	s64 counter;
//} atomic64_t;
//#endif
//
//struct list_head {
//	struct list_head *next, *prev;
//};
//
//struct hlist_head {
//	struct hlist_node *first;
//};
//
//struct hlist_node {
//	struct hlist_node *next, **pprev;
//};
//
//struct device {
//	struct device		*parent;
//};
//
//#include <stdbool.h>
//#include <stddef.h>
//
//#endif

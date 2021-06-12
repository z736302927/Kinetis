/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_PAGE_H
#define __ASM_GENERIC_PAGE_H
/*
 * Generic page.h implementation, for NOMMU architectures.
 * This provides the dummy definitions for the memory management.
 */

#ifdef CONFIG_MMU
#error need to provide a real asm/page.h
#endif


/* PAGE_SHIFT determines the page size */

#define _PAGE_SHIFT	12
#ifdef __ASSEMBLY__
#define _PAGE_SIZE	(1 << _PAGE_SHIFT)
#else
#define _PAGE_SIZE	(1UL << _PAGE_SHIFT)
#endif
#define _PAGE_MASK	(~(_PAGE_SIZE-1))

//#include <asm/setup.h>

#ifndef __ASSEMBLY__

//#define clear_page(page)	memset((page), 0, _PAGE_SIZE)
//#define copy_page(to,from)	memcpy((to), (from), _PAGE_SIZE)

//#define clear_user_page(page, vaddr, pg)	clear_page(page)
//#define copy_user_page(to, from, vaddr, pg)	copy_page(to, from)

///*
// * These are used to make use of C type-checking..
// */
//typedef struct {
//	unsigned long pte;
//} pte_t;
//typedef struct {
//	unsigned long pmd[16];
//} pmd_t;
//typedef struct {
//	unsigned long pgd;
//} pgd_t;
//typedef struct {
//	unsigned long pgprot;
//} pgprot_t;
//typedef struct page *pgtable_t;

//#define pte_val(x)	((x).pte)
//#define pmd_val(x)	((&x)->pmd[0])
//#define pgd_val(x)	((x).pgd)
//#define pgprot_val(x)	((x).pgprot)

//#define __pte(x)	((pte_t) { (x) } )
//#define __pmd(x)	((pmd_t) { (x) } )
//#define __pgd(x)	((pgd_t) { (x) } )
//#define __pgprot(x)	((pgprot_t) { (x) } )

//extern unsigned long memory_start;
//extern unsigned long memory_end;

#endif /* !__ASSEMBLY__ */

#ifdef CONFIG_KERNEL_RAM_BASE_ADDRESS
#define _PAGE_OFFSET		(CONFIG_KERNEL_RAM_BASE_ADDRESS)
#else
#define _PAGE_OFFSET		(0)
#endif

#undef ARCH_PFN_OFFSET
#define ARCH_PFN_OFFSET		(_PAGE_OFFSET >> _PAGE_SHIFT)

#ifndef __ASSEMBLY__

#define __va(x) ((void *)((unsigned long) (x)))
#define __pa(x) ((unsigned long) (x))

#define virt_to_pfn(kaddr)	(__pa(kaddr) >> _PAGE_SHIFT)
#define pfn_to_virt(pfn)	__va((pfn) << _PAGE_SHIFT)

#define virt_to_page(addr)	pfn_to_page(virt_to_pfn(addr))
#define page_to_virt(page)	pfn_to_virt(page_to_pfn(page))

#undef page_to_phys
#define page_to_phys(page)      ((dma_addr_t)page_to_pfn(page) << _PAGE_SHIFT)

#define pfn_valid(pfn)		((pfn) >= ARCH_PFN_OFFSET && ((pfn) - ARCH_PFN_OFFSET) < max_mapnr)

#define	virt_addr_valid(kaddr)	(((void *)(kaddr) >= (void *)_PAGE_OFFSET) && \
				((void *)(kaddr) < (void *)memory_end))
                
#endif /* __ASSEMBLY__ */

#include <asm-generic/memory_model.h>
#include <asm-generic/getorder.h>

#endif /* __ASM_GENERIC_PAGE_H */

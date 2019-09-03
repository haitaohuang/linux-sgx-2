/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/**
 * Copyright(c) 2016-19 Intel Corporation.
 */
#ifndef _X86_ENCL_H
#define _X86_ENCL_H

#include <linux/cpumask.h>
#include <linux/kref.h>
#include <linux/list.h>
#include <linux/mm_types.h>
#include <linux/mmu_notifier.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/radix-tree.h>
#include <linux/srcu.h>
#include <linux/workqueue.h>

/**
 * enum sgx_encl_page_desc - defines bits for an enclave page's descriptor
 * %SGX_ENCL_PAGE_TCS:			The page is a TCS page.
 * %SGX_ENCL_PAGE_ADDR_MASK:		Holds the virtual address of the page.
 *
 * The page address for SECS is zero and is used by the subsystem to recognize
 * the SECS page.
 */
enum sgx_encl_page_desc {
	SGX_ENCL_PAGE_TCS		= BIT(0),
	/* Bits 11:3 are available when the page is not swapped. */
	SGX_ENCL_PAGE_ADDR_MASK		= PAGE_MASK,
};

#define SGX_ENCL_PAGE_ADDR(page) \
	((page)->desc & SGX_ENCL_PAGE_ADDR_MASK)
#define SGX_ENCL_PAGE_VA_OFFSET(encl_page) \
	((page)->desc & SGX_ENCL_PAGE_VA_OFFSET_MASK)
#define SGX_ENCL_PAGE_IS_SECS(page) ((page) == &(page)->encl->secs)

struct sgx_encl_page {
	unsigned long desc;
	unsigned long vm_max_prot_bits;
	struct sgx_epc_page *epc_page;
	struct sgx_encl *encl;
};

enum sgx_encl_flags {
	SGX_ENCL_CREATED	= BIT(0),
	SGX_ENCL_INITIALIZED	= BIT(1),
	SGX_ENCL_DEBUG		= BIT(2),
	SGX_ENCL_DEAD		= BIT(3),
	SGX_ENCL_IOCTL		= BIT(4),
};

struct sgx_encl_mm {
	struct sgx_encl *encl;
	struct mm_struct *mm;
	struct list_head list;
	struct mmu_notifier mmu_notifier;
	struct rcu_head rcu;
};

struct sgx_encl {
	atomic_t flags;
	u64 secs_attributes;
	u64 allowed_attributes;
	unsigned int page_cnt;
	unsigned int secs_child_cnt;
	struct mutex lock;
	struct list_head mm_list;
	spinlock_t mm_lock;
	struct file *backing;
	struct kref refcount;
	struct srcu_struct srcu;
	unsigned long base;
	unsigned long size;
	unsigned long ssaframesize;
	struct radix_tree_root page_tree;
	struct sgx_encl_page secs;
	cpumask_t cpumask;
};

extern const struct vm_operations_struct sgx_vm_ops;

enum sgx_encl_mm_iter {
	SGX_ENCL_MM_ITER_DONE		= 0,
	SGX_ENCL_MM_ITER_NEXT		= 1,
	SGX_ENCL_MM_ITER_RESTART	= 2,
};

int sgx_encl_find(struct mm_struct *mm, unsigned long addr,
		  struct vm_area_struct **vma);
void sgx_encl_destroy(struct sgx_encl *encl);
void sgx_encl_release(struct kref *ref);
pgoff_t sgx_encl_get_index(struct sgx_encl_page *page);
struct page *sgx_encl_get_backing_page(struct sgx_encl *encl, pgoff_t index);
int sgx_encl_mm_add(struct sgx_encl *encl, struct mm_struct *mm);

int sgx_encl_may_map(struct sgx_encl *encl, unsigned long start,
		     unsigned long end, unsigned long vm_prot_bits);
#endif /* _X86_ENCL_H */

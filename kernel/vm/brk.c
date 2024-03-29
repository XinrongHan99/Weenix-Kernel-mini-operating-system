/******************************************************************************/
/* Important Spring 2023 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "globals.h"
#include "errno.h"
#include "util/debug.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mman.h"

#include "vm/mmap.h"
#include "vm/vmmap.h"

#include "proc/proc.h"

/*
 * This function implements the brk(2) system call.
 *
 * This routine manages the calling process's "break" -- the ending address
 * of the process's "dynamic" region (often also referred to as the "heap").
 * The current value of a process's break is maintained in the 'p_brk' member
 * of the proc_t structure that represents the process in question.
 *
 * The 'p_brk' and 'p_start_brk' members of a proc_t struct are initialized
 * by the loader. 'p_start_brk' is subsequently never modified; it always
 * holds the initial value of the break. Note that the starting break is
 * not necessarily page aligned!
 *
 * 'p_start_brk' is the lower limit of 'p_brk' (that is, setting the break
 * to any value less than 'p_start_brk' should be disallowed).
 *
 * The upper limit of 'p_brk' is defined by the minimum of (1) the
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH.
 * That is, growth of the process break is limited only in that it cannot
 * overlap with/expand into an existing mapping or beyond the region of
 * the address space allocated for use by userland. (note the presence of
 * the 'vmmap_is_range_empty' function).
 *
 * The dynamic region should always be represented by at most ONE vmarea.
 * Note that vmareas only have page granularity, you will need to take this
 * into account when deciding how to set the mappings if p_brk or p_start_brk
 * is not page aligned.
 *
 * You are guaranteed that the process data/bss region is non-empty.
 * That is, if the starting brk is not page-aligned, its page has
 * read/write permissions.
 *
 * If addr is NULL, you should "return" the current break. We use this to
 * implement sbrk(0) without writing a separate syscall. Look in
 * user/libc/syscall.c if you're curious.
 *
 * You should support combined use of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */
int
do_brk(void *addr, void **ret)
{
   	// NOT_YET_IMPLEMENTED("VM: do_brk");
   	/**
    	* adjust the size of the process's heap
    	* take single argument addr, which specifies the new end address
    	* new end addr should be greater than the current end, and aligned to the page size
    	*/

   	if (addr == NULL) {
           	*ret = curproc->p_brk;
						dbg(DBG_PRINT,"(GRADING3C 1)\n");
           	return 0;
   	}
   	uint32_t start = (uint32_t)curproc->p_start_brk;
   	uint32_t new_end = (uint32_t)addr;
   	uint32_t cur_end = (uint32_t)curproc->p_brk;
   	uint32_t i;
		dbg(DBG_PRINT,"(GRADING3C 1)\n");
   	if (new_end < start || new_end > USER_MEM_HIGH)
   	{
						dbg(DBG_PRINT,"(GRADING3D 1)\n");
           	return -ENOMEM;
   	}
   	// align the new address and the current end of the heap to the nearest page boundary
   	// uint32_t aligned_start = PAGE_ALIGN_DOWN(start);
   	uint32_t aligned_new_end = ADDR_TO_PN(PAGE_ALIGN_UP(new_end));
   	uint32_t aligned_cur_end = ADDR_TO_PN(PAGE_ALIGN_UP(cur_end));
   	uint32_t aligned_cur_start = ADDR_TO_PN(PAGE_ALIGN_DOWN(start));
		dbg(DBG_PRINT,"(GRADING3C 1)\n");
   	if (aligned_new_end == aligned_cur_end) {
           	// the new address is already aligend and the heap size does not need to be changed
           	// do nothing
						dbg(DBG_PRINT,"(GRADING3D 2)\n");
           	return 0;
   	} else {
           	// determine if the requested(new) end address is within the current heap range,
           	// regardless of whether it is page-aligned or not
           	if (new_end > cur_end) {
                   	// the new address is greater than the current end of the heap
                   	// needs to map additional memory to the heap
                   	uint32_t pages = aligned_new_end - aligned_cur_end;
										dbg(DBG_PRINT,"(GRADING3C 1)\n");
                   	if (!vmmap_is_range_empty(curproc->p_vmmap, aligned_cur_end, pages)) {
                           	// if the vmmap is not available
                           	*ret = NULL;
														dbg(DBG_PRINT,"(GRADING3D 2)\n");
                           	return -ENOMEM;
                   	}
                   	// if its available
                   	// create a new virtual memory and extend the current one
                   	vmarea_t *vma = vmmap_lookup(curproc->p_vmmap, aligned_cur_start);
                	 
                   	//dbg(DBG_TEST, "vmarea_t??? %d\n", vma == NULL);
                   	vma->vma_end = aligned_new_end;
										dbg(DBG_PRINT,"(GRADING3C 1)\n");
           	} else {
                   	// the new end address is smaller than the current end of the heap
                   	// need to unmap pages from current heap
                   	uint32_t pages = aligned_cur_end - aligned_new_end;
                   	vmmap_remove(curproc->p_vmmap, aligned_new_end, pages);
										dbg(DBG_PRINT,"(GRADING3D 1)\n");
           	}
						dbg(DBG_PRINT,"(GRADING3C 1)\n");
   	}
   	*ret = addr;
   	curproc->p_brk = addr;
		dbg(DBG_PRINT,"(GRADING3C 1)\n");
   	return 0;
}


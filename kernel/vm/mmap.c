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
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
     int fd, off_t off, void **ret)
{     
        // NOT_YET_IMPLEMENTED("VM: do_mmap");

        uint32_t lopage = 0;
        file_t *file = NULL;
        vmarea_t *vma = NULL;
        int retval = -1;
        dbg(DBG_PRINT,"(GRADING3C 1)\n");
        // Check for invalid flags
        // if ((flags & ~(MAP_SHARED | MAP_PRIVATE | MAP_FIXED | MAP_ANON)) != 0) {
        //         dbg(DBG_VMMAP,"(GRADING3B)\n");
        //         return -EINVAL;
        // }


        // check for invalid argument
        if ((int)len <= 0 || PAGE_ALIGNED(off) == 0 || PAGE_ALIGNED(addr) == 0) {
                dbg(DBG_PRINT,"(GRADING3D 1)\n");
                return -EINVAL;
        }


        if ((flags & MAP_FIXED)) {
                if ((uint32_t)addr < USER_MEM_LOW || ((uint32_t)addr + (uint32_t)len) > USER_MEM_HIGH) {
                        dbg(DBG_PRINT,"(GRADING3D 1)\n");
                        return -EINVAL;
                }
                lopage = ADDR_TO_PN((uint32_t)addr); // fixed
                dbg(DBG_PRINT,"(GRADING3D 1)\n");
        }


        // check for system file limit
        // if (fd == -EMFILE) {
        //         dbg(DBG_VMMAP,"(GRADING3B)\n");
        //         return -EMFILE;
        // }


        // check for conflicting flags
        if (!((flags & MAP_SHARED) || (flags & MAP_PRIVATE))) {
                dbg(DBG_PRINT,"(GRADING3D 1)\n");
                return -EINVAL;
        }


        // check for memory available
        // file_t *memo = fget(-1);
        // dbg(DBG_PRINT,"(GRADING3C 1)\n");
        // if (memo == NULL) {
        //         fput(memo);
        //         dbg(DBG_VMMAP,"(GRADING3B)\n");
        //         return -ENOMEM;
        // }
        
        
        //calculate page npages
        uint32_t npages = 0;
        dbg(DBG_PRINT,"(GRADING3C 1)\n");
        if (len % PAGE_SIZE == 0) {
                npages = len/PAGE_SIZE;
                dbg(DBG_PRINT,"(GRADING3C 1)\n");
        }
        else {
                npages = len/PAGE_SIZE + 1;
                dbg(DBG_PRINT,"(GRADING3D 1)\n");
        }

        // if MAP_ANON is not set
        if ((flags & MAP_ANON) != MAP_ANON) {
                dbg(DBG_PRINT,"(GRADING3C 1)\n");
                if (fd > NFILES || fd < 0) {  // we don't support fd == -1
                        dbg(DBG_PRINT,"(GRADING3D 1)\n");
                        return -EBADF;
                }     
                file = fget(fd);
                dbg(DBG_PRINT,"(GRADING3C 1)\n");
                if (file == NULL) {
                        dbg(DBG_PRINT,"(GRADING3D 1)\n");
                        return -EBADF;
                }
                if ((flags & MAP_SHARED) && (prot & PROT_WRITE) && (file->f_mode == FMODE_READ)) {
                        fput(file);
                        dbg(DBG_PRINT,"(GRADING3D 1)\n");
                        return -EACCES;
                }
                retval = vmmap_map(curproc->p_vmmap, curproc->p_files[fd]->f_vnode, lopage, npages, prot, flags, off, VMMAP_DIR_HILO, &vma);
                fput(file);
                dbg(DBG_PRINT,"(GRADING3C 1)\n");
        }
        else {
                retval = vmmap_map(curproc->p_vmmap, NULL, lopage, npages, prot, flags, off, VMMAP_DIR_HILO, &vma);
                dbg(DBG_PRINT,"(GRADING3D 2)\n");
        }
        KASSERT(NULL != curproc->p_pagedir); /* page table must be valid after a memory segment is mapped into the address space */
        dbg(DBG_PRINT,"(GRADING3A 2.a)\n");

        dbg(DBG_PRINT,"(GRADING3C 1)\n");
        //dbg(DBG_VMMAP,"retval in dommap: %d\n",retval);
        if (retval < 0) {
                dbg(DBG_PRINT,"(GRADING3D 2)\n");
                return retval;
        }
        
        // clear the TLB
        *ret = PN_TO_ADDR(vma->vma_start);
        // dbg(DBG_VMMAP,"ret:vma->vma_start %p\n",*ret);
        // dbg(DBG_VMMAP,"vlow: %d; vhigh: %d\n",(uintptr_t)PN_TO_ADDR(vma->vma_start),(uintptr_t)PN_TO_ADDR(vma->vma_start) + (uintptr_t)PAGE_ALIGN_UP(len));
        // dbg(DBG_VMMAP,"-------------------------------------------\n");
        pt_unmap_range(curproc->p_pagedir, (uintptr_t)PN_TO_ADDR(vma->vma_start), (uintptr_t)PN_TO_ADDR(vma->vma_start) + (uintptr_t)PAGE_ALIGN_UP(len));
        tlb_flush_range((uintptr_t)PN_TO_ADDR(vma->vma_start), (uint32_t)PAGE_ALIGN_UP(len) / PAGE_SIZE);
        dbg(DBG_PRINT,"(GRADING3C 1)\n");
        
        return retval;
}






/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{
        // NOT_YET_IMPLEMENTED("VM: do_munmap");
        uint32_t npages;
        dbg(DBG_PRINT,"(GRADING3D 1)\n");
        if (len % PAGE_SIZE == 0) {
                npages = len/PAGE_SIZE;
                dbg(DBG_PRINT,"(GRADING3D 1)\n");
        }
        else {
                npages = len/PAGE_SIZE + 1;
                dbg(DBG_PRINT,"(GRADING3D 1)\n");
        }
        if (PAGE_ALIGNED(addr) == 0) {
                dbg(DBG_PRINT,"(GRADING3D 1)\n");
                return -EINVAL;
        }
        if ((uint32_t)addr < USER_MEM_LOW || ((uint32_t)addr+len) > USER_MEM_HIGH) {
                dbg(DBG_PRINT,"(GRADING3D 1)\n");
                return -EINVAL;
        }
        if (len <= 0 || sizeof(len) == NULL || len > (USER_MEM_HIGH - USER_MEM_LOW)) {
                dbg(DBG_PRINT,"(GRADING3D 1)\n");
                return -EINVAL;
        }
        vmmap_remove(curproc->p_vmmap, ADDR_TO_PN((uint32_t)addr), npages);
        // if (retval < 0) {
        //         dbg(DBG_VMMAP,"(GRADING3B)\n");
        //         return retval;
        //         }
        pt_unmap_range(curproc->p_pagedir, (uintptr_t)addr, (uintptr_t)addr + (uintptr_t)PAGE_ALIGN_UP(len));
        tlb_flush_range((uintptr_t)addr, (uint32_t)PAGE_ALIGN_UP(len) / PAGE_SIZE);
        dbg(DBG_PRINT,"(GRADING3D 1)\n");
        return 0;
}



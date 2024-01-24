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

#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
        vmarea_t *parent_vma;
        pframe_t *pf;
        mmobj_t *to_delete, *new_shadowed;

        // NOT_YET_IMPLEMENTED("VM: do_fork");

        KASSERT(regs != NULL); /* the function argument must be non-NULL */
        dbg(DBG_PRINT,"(GRADING3A 7.a)\n");
        KASSERT(curproc != NULL); /* the parent process, which is curproc, must be non-NULL */
        dbg(DBG_PRINT,"(GRADING3A 7.a)\n");
        KASSERT(curproc->p_state == PROC_RUNNING); /* the parent process must be in the running state and not in the zombie state */
        dbg(DBG_PRINT,"(GRADING3A 7.a)\n");

        // 1. *Allocate a proc_t out of the procs structure using proc_create().
        proc_t *newproc = proc_create("child");
        KASSERT(newproc->p_state == PROC_RUNNING); /* new child process starts in the running state */
        dbg(DBG_PRINT,"(GRADING3A 7.a)\n");
        KASSERT(newproc->p_pagedir != NULL); /* the new child process has a valid page directory */
        dbg(DBG_PRINT,"(GRADING3A 7.a)\n");
        //dbg(DBG_TEST, "In fork, get child pid = %d\n", newproc->p_pid);

        // 2. *Copy the vmmap_t from the parent process into the child using vmmap_clone().        
        vmmap_t *vmmap_parent = curproc->p_vmmap;
        vmmap_destroy(newproc->p_vmmap);
        newproc->p_vmmap = NULL;
        vmmap_t *vmmap_child = vmmap_clone(vmmap_parent);
        
        
        //destroy the original vmmap
        dbg(DBG_TEST,"address of newproc->p_vmmap: %p\n",&newproc->p_vmmap);
        newproc->p_vmmap = vmmap_child;//把child map连到child process上
        vmmap_child->vmm_proc = newproc;//double link

        //dbginfo(DBG_VMMAP, vmmap_mapping_info, vmmap_parent);
        dbg(DBG_PRINT,"(GRADING3B 7)\n");

        list_iterate_begin(&vmmap_parent->vmm_list,parent_vma,vmarea_t,vma_plink){
                
                //find the corresponding child vma

                vmarea_t *child_vma = vmmap_lookup(vmmap_child,parent_vma->vma_start);
                //KASSERT(child_vma != NULL);
                
                //deal with bottom object
                mmobj_t *bottom_shadow_obj = mmobj_bottom_obj(parent_vma->vma_obj);
                dbg(DBG_PRINT,"(GRADING3B 7)\n");
                
                //if it is a share map(parent/child指向同一个mmobj_t,vmmap_map已经设置好了,所以只需要该refcount)
                if(child_vma->vma_flags & MAP_SHARED){
                        //*Remember to increase the reference counts on the underlying mmobj_t.
                        child_vma->vma_obj = parent_vma->vma_obj;
                        child_vma->vma_obj->mmo_ops->ref(child_vma->vma_obj);
                        list_insert_tail(&bottom_shadow_obj->mmo_un.mmo_vmas,&child_vma->vma_olink);
                        dbg(DBG_PRINT,"(GRADING3D 1)\n");
                }
                //private
                else if(child_vma->vma_flags & MAP_PRIVATE){
                        mmobj_t *original_obj = parent_vma->vma_obj;


                        mmobj_t *parent_shadow_obj = shadow_create();//new shadow object
                        mmobj_t *child_shadow_obj = shadow_create();
                        //*which in turn should point to the original mmobj_t for the vmarea_t. 
                        //*This is how you know that the pages corresponding to this mapping are copy-on-write. 
                        parent_shadow_obj->mmo_shadowed = original_obj;//这个shadow_obj是parent/child连的original obj的shadow
                        child_shadow_obj->mmo_shadowed = original_obj;
                        //bottom object
                        parent_shadow_obj->mmo_un.mmo_bottom_obj = bottom_shadow_obj;
                        child_shadow_obj->mmo_un.mmo_bottom_obj = bottom_shadow_obj;


                        // 3. *For each private mapping, point the vmarea_t at the new shadow object,
                        child_vma->vma_obj = child_shadow_obj;
                        parent_vma->vma_obj = parent_shadow_obj;


                        //*Be careful with reference counts. Also note that for shared mappings, there is no need to copy the mmobj_t.
                        //increase the reference count of shadow_obj
                        //parent_shadow_obj->mmo_ops->ref(parent_shadow_obj);
                        //child_shadow_obj->mmo_ops->ref(child_shadow_obj);
                        child_shadow_obj->mmo_un.mmo_bottom_obj->mmo_ops->ref(child_shadow_obj->mmo_un.mmo_bottom_obj);
                        parent_shadow_obj->mmo_un.mmo_bottom_obj->mmo_ops->ref(parent_shadow_obj->mmo_un.mmo_bottom_obj);
                        original_obj->mmo_ops->ref(original_obj);
                        dbg(DBG_PRINT,"(GRADING3B 7)\n");
                }
                dbg(DBG_PRINT,"(GRADING3B 7)\n");
        }list_iterate_end();
        dbg(DBG_PRINT,"(GRADING3B 7)\n");


        // *Unmap the user land page table entries and flush the TLB (using pt_unmap_range() and tlb_flush_all()).
        // *This is necessary because the parent process might still have some entries marked as "writable", but since
        // we are implementing copy-on-write we would like access to these pages to cause a trap.
        pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
        tlb_flush_all(); 

        
        // *Use kthread_clone() to copy the thread from the parent process into the child process.
        kthread_t *newthr = kthread_clone(curthr);
        KASSERT(newthr->kt_kstack != NULL); /* thread in the new child process must have a valid kernel stack */
        dbg(DBG_PRINT,"(GRADING3A 7.a)\n");

        newthr->kt_proc = newproc;//child thread->child process
        list_insert_tail(&newproc->p_threads,&newthr->kt_plink);//child process的pthreads link连newthread
        

        // *Set up the new process thread context (kt_ctx). You will need to set the following:
        // *c_pdptr - the page table pointer
        newthr->kt_ctx.c_pdptr = newproc->p_pagedir;
        // *c_eip - function pointer for the userland_entry() function
        newthr->kt_ctx.c_eip = (uintptr_t)userland_entry;
        // *c_kstack - the top of the new thread's kernel stack
        newthr->kt_ctx.c_kstack = (uintptr_t)newthr->kt_kstack;
        // *c_kstacksz - size of the new thread's kernel stack
        newthr->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;
        //ebp?
        newthr->kt_ctx.c_ebp = curthr->kt_ctx.c_ebp;
        // *Remember to set the return value in the child process
        regs->r_eax = 0;
        // *c_esp - the value returned by fork_setup_stack()
        newthr->kt_ctx.c_esp = fork_setup_stack(regs, newthr->kt_kstack);
        dbg(DBG_PRINT,"(GRADING3B 7)\n");

        // *Copy the file descriptor table of the parent into the child. Remember to use fref() here.
        for (int i = 0; i < NFILES; i++) {
                if(curproc->p_files[i]){
                        newproc->p_files[i] = curproc->p_files[i];
                        fref(newproc->p_files[i]);
                        dbg(DBG_PRINT,"(GRADING3B 7)\n");
                }
                dbg(DBG_PRINT,"(GRADING3B 7)\n");
        }
        

        // *Set any other fields in the new process which need to be set.
        newproc->p_brk = curproc->p_brk;
        newproc->p_start_brk = curproc->p_start_brk;
        // newproc->p_pagedir = curproc->p_pagedir;

        // *Make the new thread runnable.
        sched_make_runnable(newthr);
        //dbg(DBG_TEST,"end fork return pid = %d\n", newproc->p_pid);
        dbg(DBG_PRINT,"(GRADING3B 7)\n");

        return newproc->p_pid;
}

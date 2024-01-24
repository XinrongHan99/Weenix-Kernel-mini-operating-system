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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_create");
        vmmap_t *new_vmmap = (vmmap_t *) slab_obj_alloc(vmmap_allocator);
        dbg(DBG_PRINT,"(GRADING3A)\n");
        if (new_vmmap) {
                // has no vmareas
                list_init(&new_vmmap->vmm_list);
                // not refer to a process
                new_vmmap->vmm_proc = NULL;
                dbg(DBG_PRINT,"(GRADING3A)\n");
        }
        dbg(DBG_PRINT,"(GRADING3A)\n");
        return new_vmmap;
}


/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_destroy");
        KASSERT(NULL != map); /* function argument must not be NULL */
        dbg(DBG_PRINT,"(GRADING3A 3.a)\n");
        vmarea_t *vmarea;
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        // iterate through vmm_list of map: head, elem, type, member
        list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink) {
                // remove
                list_remove(&vmarea->vma_plink);
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
                // check whether the vma object is currently linked to a page table
                if (list_link_is_linked(&vmarea->vma_olink)) {
                        // if it is, removed from the page table list to avoid inconsistencies
                        list_remove(&vmarea->vma_olink);
                        dbg(DBG_PRINT,"(GRADING3B 1)\n");
                }

                mmobj_t* direct_obj = vmarea->vma_obj;
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
                if (vmarea->vma_obj != NULL) {
                        // if vma has an associated memory object, call put() to release
                        vmarea->vma_obj->mmo_ops->put(vmarea->vma_obj);
                        dbg(DBG_PRINT,"(GRADING3B 1)\n");
                }
                // free a single vmarea object
                vmarea_free(vmarea);
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
        } list_iterate_end();
        // free the vmmap struct
        slab_obj_free(vmmap_allocator, map);
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
}


/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
       // NOT_YET_IMPLEMENTED("VM: vmmap_insert");
        KASSERT(NULL != map && NULL != newvma); /* both function arguments must not be NULL */
        dbg(DBG_PRINT,"(GRADING3A 3.b)\n");
        KASSERT(NULL == newvma->vma_vmmap); /* newvma must be newly create and must not be part of any existing vmmap */
        dbg(DBG_PRINT,"(GRADING3A 3.b)\n");
        KASSERT(newvma->vma_start < newvma->vma_end); /* newvma must not be empty */
        dbg(DBG_PRINT,"(GRADING3A 3.b)\n");
        KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
                                  /* addresses in this memory segment must lie completely within the user space */
        dbg(DBG_PRINT,"(GRADING3A 3.b)\n");
        vmarea_t *curr = NULL;
        newvma->vma_vmmap = map;
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        list_iterate_begin(&map->vmm_list, curr, vmarea_t, vma_plink) {
                if(curr->vma_start >= newvma->vma_end){
                        list_insert_before(&curr->vma_plink, &newvma->vma_plink);
                        dbg(DBG_PRINT,"(GRADING3B 1)\n");
                        return;
                }
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
        } list_iterate_end();

        list_insert_tail(&map->vmm_list, &newvma->vma_plink);
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
}


/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{      
       // NOT_YET_IMPLEMENTED("VM: vmmap_find_range");
       uint32_t pages;
       dbg(DBG_PRINT,"(GRADING3C 1)\n");
//        if (dir == VMMAP_DIR_HILO) {
               vmarea_t *vma;
               dbg(DBG_PRINT,"(GRADING3C 1)\n");
               // 倒着找
               list_iterate_reverse(&map->vmm_list, vma, vmarea_t, vma_plink) {
                       // 最后一个
                       if (vma->vma_plink.l_next == &map->vmm_list) {
                               pages = ADDR_TO_PN(USER_MEM_HIGH) - vma->vma_end; // end: exclusive
                               dbg(DBG_PRINT,"(GRADING3C 1)\n");
                               if (pages >= npages) {
                                        dbg(DBG_PRINT,"(GRADING3C 1)\n");
                                       return (ADDR_TO_PN(USER_MEM_HIGH) - npages);
                                }
                                dbg(DBG_PRINT,"(GRADING3D 1)\n");
                       }
                       else { // 在中间找
                               vmarea_t *next = list_item(vma->vma_plink.l_next, vmarea_t, vma_plink);
                               pages = next->vma_start - vma->vma_end;
                               dbg(DBG_PRINT,"(GRADING3D 1)\n");
                               if (pages >= npages) {
                                        dbg(DBG_PRINT,"(GRADING3D 1)\n");
                                       return (next->vma_start - npages);
                               }
                               dbg(DBG_PRINT,"(GRADING3D 1)\n");
                       }
               } list_iterate_end();
               // 第一个
               pages = vma->vma_start - ADDR_TO_PN(USER_MEM_LOW); // start: inclusive
               dbg(DBG_PRINT,"(GRADING3D 2)\n");
               if (pages >= npages) {
                        dbg(DBG_PRINT,"(GRADING3D 2)\n");
                       return (vma->vma_start - npages);
               }
               else {
                        dbg(DBG_PRINT,"(GRADING3D 2)\n");
                       return -1; // 找不到
               }
//        }
//        else if (dir == VMMAP_DIR_LOHI) {
//                vmarea_t *vma;
//                dbg(DBG_VMMAP,"(GRADING3B)\n");
//                list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
//                        // 第一个
//                        if (vma->vma_plink.l_prev == &map->vmm_list) {
//                                pages = vma->vma_start - ADDR_TO_PN(USER_MEM_LOW);
//                                dbg(DBG_VMMAP,"(GRADING3B)\n");
//                                if (pages >= npages) {
//                                         dbg(DBG_VMMAP,"(GRADING3B)\n");
//                                        return ADDR_TO_PN(USER_MEM_LOW);
//                                }
//                                dbg(DBG_VMMAP,"(GRADING3B)\n");
//                        }
//                        else { // 在中间找
//                                vmarea_t *prev = list_item(vma->vma_plink.l_prev, vmarea_t, vma_plink);
//                                pages = vma->vma_start - prev->vma_end;
//                                dbg(DBG_VMMAP,"(GRADING3B)\n");
//                                if (pages >= npages) {
//                                         dbg(DBG_VMMAP,"(GRADING3B)\n");
//                                        return prev->vma_end;
//                                }
//                                dbg(DBG_VMMAP,"(GRADING3B)\n");
//                        }
//                } list_iterate_end();
//                // 最后一个
//                pages = ADDR_TO_PN(USER_MEM_HIGH) - vma->vma_end;
//                dbg(DBG_VMMAP,"(GRADING3B)\n");
//                if (pages >= npages) {
//                         dbg(DBG_VMMAP,"(GRADING3B)\n");
//                        return vma->vma_end;
//                }
//                else {
//                         dbg(DBG_VMMAP,"(GRADING3B)\n");
//                        return -1; // 找不到
//                }
//        }
//        dbg(DBG_VMMAP,"(GRADING3B)\n");
//        return -1; // 有可能不是这两种中的一个吗？
}


/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{

        // NOT_YET_IMPLEMENTED("VM: vmmap_lookup");
        KASSERT(NULL != map); /* the first function argument must not be NULL */
        dbg(DBG_PRINT,"(GRADING3A 3.c)\n");
        vmarea_t *vma = NULL;
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                if ((vfn >= vma->vma_start) && (vfn < vma->vma_end)) {
                        dbg(DBG_PRINT,"(GRADING3B 1)\n");
                        return vma;
                }
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
        } list_iterate_end();
        dbg(DBG_PRINT,"(GRADING3C 5)\n");
        return NULL;
}


/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_clone");
        vmmap_t *clone = vmmap_create(); // 有可能 create 不成功吗？若不成功会返回什么？
        vmarea_t *vma;
        dbg(DBG_PRINT,"(GRADING3B 7)\n");
        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                vmarea_t *new_vma = vmarea_alloc();
                new_vma->vma_start = vma->vma_start;
                new_vma->vma_end = vma->vma_end;
                new_vma->vma_off = vma->vma_off;
                new_vma->vma_prot = vma->vma_prot;
                new_vma->vma_flags = vma->vma_flags;
                list_link_init(&new_vma->vma_plink);
                list_link_init(&new_vma->vma_olink);
                vmmap_insert(clone,new_vma);
                new_vma->vma_obj = NULL;
                dbg(DBG_PRINT,"(GRADING3B 7)\n");
        } list_iterate_end();
        dbg(DBG_PRINT,"(GRADING3B 7)\n");
        return clone;
}


/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
         int prot, int flags, off_t off, int dir, vmarea_t **new)
{      
       // NOT_YET_IMPLEMENTED("VM: vmmap_map");
       KASSERT(NULL != map); /* must not add a memory segment into a non-existing vmmap */
       dbg(DBG_PRINT,"(GRADING3A 3.d)\n");
       KASSERT(0 < npages); /* number of pages of this memory segment cannot be 0 */
       dbg(DBG_PRINT,"(GRADING3A 3.d)\n");
       KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags)); /* must specify whether the memory segment is shared or private */
       dbg(DBG_PRINT,"(GRADING3A 3.d)\n");
       KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage)); /* if lopage is not zero, it must be a user space vpn */
       dbg(DBG_PRINT,"(GRADING3A 3.d)\n");
       KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
       dbg(DBG_PRINT,"(GRADING3A 3.d)\n");
       KASSERT(PAGE_ALIGNED(off)); /* the off argument must be page aligned */
       dbg(DBG_PRINT,"(GRADING3A 3.d)\n");

       vmarea_t *vma = vmarea_alloc();
       dbg(DBG_PRINT,"(GRADING3B 1)\n");
       if (vma == NULL){ 
        dbg(DBG_VMMAP,"(GRADING3B)\n");
        return -1;
        }
        dbg(DBG_PRINT,"(GRADING3B 1)\n");

       // lopage is zero
       if (lopage == 0) {
               int retval = vmmap_find_range(map, npages, dir);
               if (retval == -1) return -1;
               lopage = (uint32_t)retval;
               dbg(DBG_PRINT,"(GRADING3C 1)\n");
       }
       // lopage is nonzero
       else {
               // the specified region contains another mapping, then unmapped
               if (!vmmap_is_range_empty(map, lopage, npages)) {
                       vmmap_remove(map, lopage, npages);
                       dbg(DBG_PRINT,"(GRADING3B 1)\n");
               }
               dbg(DBG_PRINT,"(GRADING3B 1)\n");
       }
      
       vma->vma_start = lopage;
       vma->vma_end = lopage + npages;
       vma->vma_off = ADDR_TO_PN(off);
       vma->vma_prot = prot;
       vma->vma_flags = flags;
       vma->vma_obj = NULL;
       list_link_init(&vma->vma_plink);
       list_link_init(&vma->vma_olink);
      
       mmobj_t *obj;
       dbg(DBG_PRINT,"(GRADING3B 1)\n");


       if (file == NULL) {
               obj = anon_create();
               dbg(DBG_PRINT,"(GRADING3B 1)\n");
       }
       else {
               file->vn_ops->mmap(file, vma, &obj);
               dbg(DBG_PRINT,"(GRADING3B 1)\n");
       }

        dbg(DBG_PRINT,"(GRADING3B 1)\n");
       // if MAP_PRIVATE is specified set up a shadow object for the mmobj
       if (flags & MAP_PRIVATE) {
               mmobj_t *shadow_obj = shadow_create();


               // The mmo_shadowed field of the shadow_obj is set to the original memory object
               shadow_obj->mmo_shadowed = obj;


               // The mmo_un.mmo_bottom_obj field of shadow_obj is set to
               // the bottom object of the original memory object using the mmobj_bottom_obj macro
               mmobj_t *bottom_obj = mmobj_bottom_obj(obj);
               shadow_obj->mmo_un.mmo_bottom_obj = bottom_obj;


               // The vma_obj field of vma is set to the new shadow object
               vma->vma_obj = shadow_obj;


               // inserts the vma into the list of VMAs that share the same bottom-level memory object
               list_insert_head(&(bottom_obj->mmo_un.mmo_vmas), &(vma->vma_olink));


               //increase refcount of bottom object:
               bottom_obj->mmo_ops->ref(bottom_obj);
               dbg(DBG_PRINT,"(GRADING3B 1)\n");
       }
       else {
               vma->vma_obj = obj;
               list_insert_head(&(obj->mmo_un.mmo_vmas), &(vma->vma_olink));
               dbg(DBG_PRINT,"(GRADING3D 1)\n");
       }


       vmmap_insert(map, vma);

        dbg(DBG_PRINT,"(GRADING3B 1)\n");
       // if 'new' is non-NULL a pointer to the new vmarea_t should be stored in it
       if (new != NULL) {
               *new = vma;
               dbg(DBG_PRINT,"(GRADING3C 1)\n");
       }
       dbg(DBG_PRINT,"(GRADING3B 1)\n");
       return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_remove");
        uint32_t start_pf = lopage;
        uint32_t end_pf = lopage + npages;
        
        //dbg(DBG_TEST,"start remove: address to remove %p - %p \n",start_pf,end_pf);
        vmarea_t *cur_vma = NULL;
        // dbginfo(DBG_VMMAP, vmmap_mapping_info, map);
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        list_iterate_begin(&map->vmm_list,cur_vma,vmarea_t,vma_plink){
                //超过结尾了
                if(cur_vma->vma_start >= end_pf){
                        dbg(DBG_PRINT,"(GRADING3D 1)\n");
                        break;
                }

                //还没到开头
                if(cur_vma->vma_end < start_pf){
                        dbg(DBG_PRINT,"(GRADING3D 1)\n");
                        continue;
                }
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
                if(cur_vma->vma_start < start_pf && cur_vma->vma_end > end_pf){
                        //case1: split the VMA into two seperate VMAs
                        vmarea_t *new_vma = vmarea_alloc();
                        // *new_vma = *cur_vma;
                        new_vma->vma_obj = cur_vma->vma_obj;
                        new_vma->vma_flags = cur_vma->vma_flags;
                        new_vma->vma_off = cur_vma->vma_off + end_pf - cur_vma->vma_start;
                        list_init(&new_vma->vma_olink);
                        list_init(&new_vma->vma_plink);
                        new_vma->vma_prot = cur_vma->vma_prot;
                        //new_vma->vma_vmmap = cur_vma->vma_vmmap;
                        new_vma->vma_start = end_pf;
                        new_vma->vma_end = cur_vma->vma_end;

                        cur_vma->vma_end = start_pf;

                        vmmap_insert(map,new_vma);

                        //list_insert_tail(&cur_vma->vma_plink,&new_vma->vma_plink);
                        //increase the reference count
                        new_vma->vma_obj->mmo_ops->ref(new_vma->vma_obj);
                        dbg(DBG_PRINT,"(GRADING3D 2)\n");
                }else if(start_pf > cur_vma->vma_start && end_pf >= cur_vma->vma_end ){//&& start_pf < cur_vma->vma_end
                        //case2: shorten the length of VMA
                        cur_vma->vma_end = start_pf;
                        dbg(DBG_PRINT,"(GRADING3B 1)\n");
                }else if(start_pf <= cur_vma->vma_start && end_pf < cur_vma->vma_end){
                        //case3: move the beginning fo the vma and shorten its length
                        cur_vma->vma_off += (end_pf - cur_vma->vma_start);
                        cur_vma->vma_start = end_pf;
                        dbg(DBG_PRINT,"(GRADING3D 2)\n");
                }else if(start_pf<= cur_vma->vma_start && end_pf >= cur_vma->vma_end){
                        //case4: remove the vma from the list
                        list_remove(&cur_vma->vma_plink);
                        cur_vma->vma_obj->mmo_ops->put(cur_vma->vma_obj);
                        dbg(DBG_PRINT,"(GRADING3B 1)\n");
                        if (list_link_is_linked(&cur_vma->vma_olink)) {
                                // if it is, removed from the page table list to avoid inconsistencies
                                list_remove(&cur_vma->vma_olink);
                                dbg(DBG_PRINT,"(GRADING3B 1)\n");
                        }       
                        vmarea_free(cur_vma);
                        dbg(DBG_PRINT,"(GRADING3B 1)\n");
                        continue;
                }
        }list_iterate_end();
        dbg(DBG_PRINT,"(GRADING3B 1)\n");

        return 0;

}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty");
        uint32_t endvfn = startvfn + npages;
        KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
                                  /* the specified page range must not be empty and lie completely within the user space */
        dbg(DBG_PRINT,"(GRADING3A 3.e)\n");
        vmarea_t *cur_vma = NULL;
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        list_iterate_begin(&map->vmm_list,cur_vma,vmarea_t,vma_plink){
                if(!(cur_vma->vma_start >= endvfn || cur_vma->vma_end <= startvfn)){
                        dbg(DBG_PRINT,"(GRADING3B 1)\n");
                        return 0;
                }
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
        }list_iterate_end();
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_read");
        uint32_t end_vaddr = (uint32_t)vaddr + (uint32_t)count;             // copy range is [vaddr, end_vaddr)

        uint32_t start_vfn = ADDR_TO_PN(vaddr);
        uint32_t end_vfn = ADDR_TO_PN(end_vaddr-1);     // copy range is [vaddr, end_vaddr-1]

        uint32_t offset = PAGE_OFFSET(vaddr);
        uint32_t already_read = 0;

        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        for(uint32_t cur_vfn = start_vfn; cur_vfn <= end_vfn; cur_vfn++){
                // 1. find the pframe of the cur_vfn
                vmarea_t *vma = vmmap_lookup(map, cur_vfn);
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
                if (vma == NULL) {
                        dbg(DBG_VMMAP,"(GRADING3B)\n");
                        return -EFAULT;
                }
                
                pframe_t *pf;
                int err = pframe_lookup(vma->vma_obj, vma->vma_off + (cur_vfn - vma->vma_start) , 0, &pf);
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
                if (err) {
                        dbg(DBG_VMMAP,"(GRADING3B)\n");
                        return err;
                }

                // 2. copy data
                uint32_t cur_addr_start_copy = offset + (uint32_t)pf->pf_addr;
                offset = 0;
                uint32_t end_addr_of_pf = (uint32_t)pf->pf_addr + PAGE_SIZE;                      // [cur_addr_start_copy, end_addr_of_pf)
                uint32_t max_amount_copy = end_addr_of_pf - cur_addr_start_copy;
                uint32_t real_amount_copy = MIN(max_amount_copy,count);

                memcpy((void *)((uint32_t)buf + already_read), (const void*)cur_addr_start_copy, real_amount_copy);
                count -= real_amount_copy;
                already_read += real_amount_copy;
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
        }
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        return 0;
}


/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
        // NOT_YET_IMPLEMENTED("VM: vmmap_write");
        uint32_t end_vaddr = (uint32_t)vaddr + (uint32_t)count;             // copy range is [vaddr, end_vaddr)

        uint32_t start_vfn = ADDR_TO_PN(vaddr);
        uint32_t end_vfn = ADDR_TO_PN(end_vaddr-1);     // copy range is [vaddr, end_vaddr-1]

        uint32_t offset = PAGE_OFFSET(vaddr);
        uint32_t already_write = 0;

        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        for(uint32_t cur_vfn = start_vfn; cur_vfn <= end_vfn; cur_vfn++){
                // 1. find the pframe of the cur_vfn
                vmarea_t *vma = vmmap_lookup(map, cur_vfn);
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
                if (vma == NULL) {
                        dbg(DBG_VMMAP,"(GRADING3B)\n");
                        return -EFAULT;
                }
                
                pframe_t *pf;
                int err = pframe_lookup(vma->vma_obj, vma->vma_off + (cur_vfn - vma->vma_start) , 1, &pf);
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
                if (err) {
                        dbg(DBG_VMMAP,"(GRADING3B)\n");
                        return err;
                }

                // 2. copy data
                uint32_t cur_addr_start_copy = offset + (uint32_t)pf->pf_addr;
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
                if (offset != 0) {
                        dbg(DBG_PRINT,"(GRADING3B 1)\n");
                        offset = 0;
                }
                uint32_t end_addr_of_pf = (uint32_t)pf->pf_addr + PAGE_SIZE;                      // [cur_addr_start_copy, end_addr_of_pf)
                uint32_t max_amount_copy = end_addr_of_pf - cur_addr_start_copy;
                uint32_t real_amount_copy = MIN(max_amount_copy,count);
 
                pframe_dirty(pf);
                
                memcpy((void *)cur_addr_start_copy, (const void*)((uint32_t)buf + already_write), real_amount_copy);

                count -= real_amount_copy;
                already_write += real_amount_copy;
                dbg(DBG_PRINT,"(GRADING3B 1)\n");
        }
        dbg(DBG_PRINT,"(GRADING3B 1)\n");
        return 0;

}

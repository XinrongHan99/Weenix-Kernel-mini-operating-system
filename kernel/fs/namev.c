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
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
       //NOT_YET_IMPLEMENTED("VFS: lookup");
        KASSERT(NULL != dir); /* the "dir" argument must be non-NULL */
        //dbg(DBG_PRINT, "(GRADING2A 2.a)\n");

        KASSERT(NULL != name); /* the "name" argument must be non-NULL */
        //dbg(DBG_PRINT, "(GRADING2A 2.a)\n");

        KASSERT(NULL != result); /* the "result" argument must be non-NULL */
        //dbg(DBG_PRINT, "(GRADING2A 2.a)\n");


        if(!S_ISDIR(dir->vn_mode)){
                //dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTDIR;
        }

        if(!dir->vn_ops->lookup){
                //dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENOTDIR;
        }
        if (len > NAME_LEN) {
                //dbg(DBG_PRINT, "(GRADING2B)\n");
                return -ENAMETOOLONG;
        }
        if(len == 0){
                //dbg(DBG_PRINT, "(GRADING2B)\n");
                return -EEXIST;
        }
        if(len == 1 && *name == '/' && dir == vfs_root_vn){
                *result = vfs_root_vn;
                vref(vfs_root_vn);
                //dbg(DBG_PRINT, "(GRADING2B)\n");
                return 0;
                // return vfs_root_vn->vn_mmobj.mmo_refcount;
        }
        //dbg(DBG_PRINT, "(GRADING2B)\n");
        return dir->vn_ops->lookup(dir, name, len, result);
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int
dir_namev(const char *pathname, size_t *namelen, const char **name,
          vnode_t *base, vnode_t **res_vnode)
{
        // NOT_YET_IMPLEMENTED("VFS: dir_namev");
        KASSERT(NULL != pathname); /* the "pathname" argument must be non-NULL */
        //dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

        KASSERT(NULL != namelen); /* the "namelen" argument must be non-NULL */
        //dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

        KASSERT(NULL != name); /* the "name" argument must be non-NULL */
        //dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

        KASSERT(NULL != res_vnode); /* the "res_vnode" argument must be non-NULL */
        //dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

        //set the start point of path resolution
        vnode_t *cur_vnode;
        int res;

        if(pathname[0] == '/'){
                cur_vnode = vfs_root_vn;
                //dbg(DBG_PRINT, "(GRADING2B)\n");
        }else if(base != NULL){
                cur_vnode = base;
                //dbg(DBG_PRINT, "(GRADING2B)\n");
        }else{
                cur_vnode = curproc->p_cwd;
                //dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        vref(cur_vnode);//successful call to this causes vnode refcount on *res_vnode to be incremented

        KASSERT(NULL != cur_vnode); /* pathname resolution must start with a valid directory */
        //dbg(DBG_PRINT, "(GRADING2A 2.b)\n");

        //path resolution
        const char *path_ptr = pathname;
        const char *start;
        char *end;

        const char* substrs[MAXPATHLEN];
        size_t substr_len[MAXPATHLEN];
        int substr_cnt = 0;
        int flag = 0;

        while (1)
        {
                // find the first non-'/'
                while (*path_ptr && *path_ptr == '/') //      "/../"     "a/b/c/"
                {
                        if(*(path_ptr + 1) == '\0' && *(path_ptr-1)!='\0'){
                                flag = 1;
                        }
                        path_ptr++;
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
                }

                if(*path_ptr == NULL)          // case "///////a//" or "/"
                {
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
                        break;
                }
                substrs[substr_cnt] = path_ptr;

                start = path_ptr;               
                end = strchr(start,'/');
                //dbg(DBG_PRINT, "(GRADING2B)\n");
                
                if (end == NULL) 
                {
                        substr_len[substr_cnt++] = strlen(start);
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
                        break;
                } else
                {
                        substr_len[substr_cnt++] = end - start;
                        path_ptr = end;
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
        
                }
                //dbg(DBG_PRINT, "(GRADING2B)\n");
        } 
     
        //dbg(DBG_PRINT, "(GRADING2B)\n");

        if (substr_cnt == 0)           // case "/////////" or "/"
        {
                *namelen = 1;
                *name = pathname;
                *res_vnode = cur_vnode;
                //dbg(DBG_PRINT, "(GRADING2B)\n");
                return 0;
        }

        vnode_t *next_vnode;
        //dbg(DBG_PRINT, "(GRADING2B)\n");
      
        for (int i = 0; i < substr_cnt - 1; i++)
        {
                res = lookup(cur_vnode, substrs[i], substr_len[i], &next_vnode);  // store the result of
                vput(cur_vnode);        //decrease reference count
                //dbg(DBG_PRINT, "(GRADING2B)\n");

                if(res)
                {
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
                        return res;
                }
                cur_vnode = next_vnode;
                //dbg(DBG_PRINT, "(GRADING2B)\n");
        }

        if(flag){
                //dbg(DBG_PRINT, "(GRADING2B)\n");
                vnode_t *final_vnode;
                int final_lookup = lookup(cur_vnode, substrs[substr_cnt - 1], substr_len[substr_cnt - 1], &final_vnode);
                if (final_lookup == 0) {
                        vput(final_vnode);
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
                }
                if(!final_vnode->vn_ops->lookup||!S_ISDIR(final_vnode->vn_mode)){
                        vput(cur_vnode);
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
                        return -ENOTDIR;
                }
                //dbg(DBG_PRINT, "(GRADING2B)\n");    
        }
        
        *namelen = substr_len[substr_cnt - 1];
        *name = substrs[substr_cnt - 1];
        *res_vnode = cur_vnode;
        //dbg(DBG_PRINT, "(GRADING2B)\n");
        return 0; 

}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified and the file does
 * not exist, call create() in the parent directory vnode. However, if the
 * parent directory itself does not exist, this function should fail - in all
 * cases, no files or directories other than the one at the very end of the path
 * should be created.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
       //NOT_YET_IMPLEMENTED("VFS: open_namev");
        vnode_t *dir_vnode;
        size_t name_len = 0;
        const char *name = NULL;

        int retval = dir_namev(pathname, &name_len, &name, base, &dir_vnode);
        //dbg(DBG_PRINT, "(GRADING2B)\n");

        if (retval != 0) {
                //dbg(DBG_PRINT, "(GRADING2B)\n");
                return retval;
        }
        else {
               int lookup_retval = lookup(dir_vnode, name, name_len, res_vnode);

               if (lookup_retval != 0) {
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
                        if ((flag & O_CREAT) && (lookup_retval == -ENOENT)) {
                                KASSERT(NULL != dir_vnode->vn_ops->create);
                                //dbg(DBG_PRINT, "(GRADING2A 2.c)\n");
                                
                                int create_retval = dir_vnode->vn_ops->create(dir_vnode, name, name_len, res_vnode);
                                vput(dir_vnode);
                                //dbg(DBG_PRINT, "(GRADING2B)\n");
                                return create_retval;
                        }
                        vput(dir_vnode);
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
                        return lookup_retval;
               }
               else {
                        vput(dir_vnode);
                        //dbg(DBG_PRINT, "(GRADING2B)\n");
                        return 0;
               }
               //dbg(DBG_PRINT, "(GRADING2B)\n");
        }
        //dbg(DBG_PRINT, "(GRADING2B)\n");
}


#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */

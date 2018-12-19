#include "crypt.h"

#define FUSE_USE_VERSION 30
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

char key[16];

static size_t init_path_buffer(const char * const path, char buff[PATH_MAX])
{
    memset(buff, 0, PATH_MAX);
    strncpy(buff, path, PATH_MAX);
    size_t len = strlen(path);

    return len;
}

static void crypt_uncrypt_path(char *buff, size_t len)
{
    size_t remaining = 16 - (len % 16);
    do_crypt(buff, len + remaining, key);
}


static const char *root_dir;

static void get_full_path(const char *path, char buff[PATH_MAX], bool encrypted)
{
    strcpy(buff, root_dir);
    strncat(buff, path, PATH_MAX);
    if (encrypted)
        crypt_uncrypt_path(buff + strlen(root_dir), strlen(path));
}

int my_getattr(const char *path, struct stat *statbuf)
{
    char full_path[PATH_MAX];
    get_full_path(path, full_path, strcmp(path, "/") != 0);
    //crypt_uncrypt_path(strrchr(full_path, '/'));

    return stat(full_path, statbuf);
}

int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    char full_path[PATH_MAX];
    get_full_path(path, full_path, strcmp(path, "/") != 0);
    DIR *dir_ptr = opendir(full_path);
    struct dirent *dirent;
    dirent = readdir(dir_ptr);
    do
    {
        char buff[PATH_MAX];
        strncpy(buff, dirent->d_name, PATH_MAX);
        crypt_uncrypt_path(buff, strlen(dirent->d_name));
        if (filler(buf, buff, NULL, 0) != 0)
        {
            return -ENOMEM;
        }
    }
    while ((dirent = readdir(dir_ptr)) != NULL);
    
    return 0;
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int bb_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_opendir(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    bb_fullpath(fpath, path);

    // since opendir returns a pointer, takes some custom handling of
    // return status.
    dp = opendir(fpath);
    log_msg("    opendir returned 0x%p\n", dp);
    if (dp == NULL)
	retstat = log_error("bb_opendir opendir");
    
    fi->fh = (intptr_t) dp;
    
    log_fi(fi);
    
    return retstat;
}

int bb_mkdir(const char *path, mode_t mode)
{
    char real_path[PATH_MAX];
    size_t len = init_path_buffer(path, real_path);
    crypt_uncrypt_path(real_path, len);

    return mkdir(real_path, mode);
}

int bb_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
	  path, mode, dev);
    bb_fullpath(fpath, path);
    
    // On Linux this could just be 'mknod(path, mode, dev)' but this
    // tries to be be more portable by honoring the quote in the Linux
    // mknod man page stating the only portable use of mknod() is to
    // make a fifo, but saying it should never actually be used for
    // that.
    if (S_ISREG(mode)) {
	retstat = log_syscall("open", open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode), 0);
	if (retstat >= 0)
	    retstat = log_syscall("close", close(retstat), 0);
    } else
	if (S_ISFIFO(mode))
	    retstat = log_syscall("mkfifo", mkfifo(fpath, mode), 0);
	else
	    retstat = log_syscall("mknod", mknod(fpath, mode, dev), 0);
    
    return retstat;
}

/** Remove a file */
int bb_unlink(const char *path)
{
    char fpath[PATH_MAX];
    
    log_msg("bb_unlink(path=\"%s\")\n",
	    path);
    bb_fullpath(fpath, path);

    return log_syscall("unlink", unlink(fpath), 0);
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.
int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    return pread(fi->fh, buf, size, offset);
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int bb_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    return pwrite(fi->fh, buf, size, offset);
}

static struct fuse_operations operations = 
{
    .getattr	= my_getattr,
    .readdir	= my_readdir,
};


void usage(const char * const name)
{
    fprintf(stderr, "Usage: %s <root dir> <mount point>\n", name);
    exit(1);
}


int main(int argc, char *argv[])
{
    fprintf(stderr, "Fuse library version %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);
    if ((argc < 3) || (argv[argc - 2][0] == '-') || (argv[argc - 1][0] == '-'))
	    usage(argv[0]);

    root_dir = realpath(argv[argc-2], NULL);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    return fuse_main(argc, argv, &operations, NULL);
}
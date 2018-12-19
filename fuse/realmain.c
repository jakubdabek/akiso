#include "ssl-crypt.h"
#include "path_util.h"

#define FUSE_USE_VERSION 30
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

const char *root_dir;
size_t root_dir_len;

cipher_t key[16];
cipher_t *iv  = (cipher_t*)"0123456789012345"; //meh

int my_getattr(const char *path, struct stat *statbuf)
{
    cipher_t full_path[PATH_MAX];
    get_real_path(path, full_path, key, iv);
    fprintf(stderr, "getattr(%s): real_path: \"%s\"\n", path, full_path);

    return stat((const char*)full_path, statbuf);
}

int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    cipher_t full_path[PATH_MAX];
    get_real_path(path, full_path, key, iv);
    fprintf(stderr, "readdir(%s): real_path: \"%s\"\n", path, full_path);
    DIR *dir_ptr = opendir((const char*)full_path);
    struct dirent *dirent;
    dirent = readdir(dir_ptr);
    do
    {
        cipher_t buff[PATH_MAX];
        my_encrypt_base64(dirent->d_name, strlen(dirent->d_name), key, iv, buff);
        fprintf(stderr, "got: \"%s\", giving directory: \"%s\"\n", dirent->d_name, buff);
        if (filler(buf, (const char*)buff, NULL, 0) != 0)
        {
            return -ENOMEM;
        }
    }
    while ((dirent = readdir(dir_ptr)) != NULL);
    
    return 0;
}

int my_mkdir(const char *path, mode_t mode)
{
    cipher_t real_path[PATH_MAX];
    my_encrypt_base64(path, strlen(path), key, iv, real_path);

    return mkdir((const char*)real_path, mode);
}

int my_mknod(const char *path, mode_t mode, dev_t dev)
{
    cipher_t real_path[PATH_MAX];
    my_encrypt_base64(path, strlen(path), key, iv, real_path);

    return mknod((const char*)real_path, mode, dev);
}

int my_unlink(const char *path)
{
    cipher_t real_path[PATH_MAX];
    my_encrypt_base64(path, strlen(path), key, iv, real_path);

    return unlink((const char*)real_path);
}

int my_rmdir(const char *path)
{
    cipher_t real_path[PATH_MAX];
    my_encrypt_base64(path, strlen(path), key, iv, real_path);

    return rmdir((const char*)real_path);
}

int my_open(const char *path, struct fuse_file_info *fi)
{
    cipher_t real_path[PATH_MAX];
    my_encrypt_base64(path, strlen(path), key, iv, real_path);
    int fd = open((const char*)real_path, fi->flags);
	
    if (fd >= 0)
        fi->fh = fd;
    
    return fd;
}

int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    //TODO: encrypt data
    return pread(fi->fh, buf, size, offset);
}

int my_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    //TODO: encrypt data
    return pwrite(fi->fh, buf, size, offset);
}

int my_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    if (datasync)
	    return fdatasync(fi->fh);
    else
	    return fsync(fi->fh);
}

static struct fuse_operations operations = 
{
    .getattr	= my_getattr,
    .readdir	= my_readdir,
    .mknod      = my_mknod,
    .unlink     = my_unlink,
    .mkdir      = my_mkdir,
    .rmdir      = my_rmdir,
    .open       = my_open,
    .read       = my_read,
    .write      = my_write,
    .fsync      = my_fsync,
    .readdir    = my_readdir,
};

void usage(const char * const name)
{
    fprintf(stderr, "Usage: %s <root dir> <key> <mount point>\n", name);
    exit(1);
}


int main(int argc, char *argv[])
{
    fprintf(stderr, "Fuse library version %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);
    if ((argc < 3) ||
        (argv[argc - 3][0] == '-') ||
        (argv[argc - 2][0] == '-') ||
        (argv[argc - 1][0] == '-'))
	    usage(argv[0]);

    strncpy((char*)key, argv[argc-2], 16);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    root_dir = realpath(argv[argc-2], NULL);
    root_dir_len = strlen(root_dir);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    return fuse_main(argc, argv, &operations, NULL);
}
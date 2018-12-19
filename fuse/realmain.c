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

static FILE *log_file;

int my_getattr(const char *path, struct stat *statbuf)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "getattr(%s): real_path: \"%s\"\n", path, real_path);

    return stat((const char*)real_path, statbuf);
}

int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "readdir(%s): real_path: \"%s\"\n", path, real_path);
    DIR *dir_ptr = opendir((const char*)real_path);
    struct dirent *dirent;
    dirent = readdir(dir_ptr);
    do
    {
        char buff[PATH_MAX];
        if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
        {
            strcpy((char*)buff, dirent->d_name);
        }
        else
        {
            my_decrypt_base64((const cipher_t*)dirent->d_name, strlen(dirent->d_name), key, iv, buff);
        }
        fprintf(log_file, "got: \"%s\", giving directory: \"%s\"\n", dirent->d_name, buff);
        if (filler(buf, buff, NULL, 0) != 0)
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
    get_real_path(path, real_path, key, iv);

    return mkdir((const char*)real_path, mode);
}

int my_mknod(const char *path, mode_t mode, dev_t dev)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "mknod(%s): real_path: \"%s\"\n", path, real_path);

    return mknod((const char*)real_path, mode, dev);
}

int my_unlink(const char *path)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);

    return unlink((const char*)real_path);
}

int my_rmdir(const char *path)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);

    return rmdir((const char*)real_path);
}

int my_open(const char *path, struct fuse_file_info *fi)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "open(%s): real_path: \"%s\"\n", path, real_path);
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

int my_chmod(const char *path, mode_t mode)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    return chmod((const char*)real_path, mode);
}

int my_chown(const char *path, uid_t uid, gid_t gid)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    return chown((const char*)real_path, uid, gid);
}

int my_truncate(const char *path, off_t newsize)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);

    return truncate((const char*)real_path, newsize);
}

int my_utime(const char *path, struct utimbuf *ubuf)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);

    return utime((const char*)real_path, ubuf);
}

int my_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    fprintf(log_file, "fgetattr(%s)\n", path);
    return fstat(fi->fh, statbuf);
}

int my_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    fprintf(log_file, "ftruncate(%s)\n", path);
    return ftruncate(fi->fh, offset);
}

int my_access(const char *path, int mask)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "access(%s): real_path: \"%s\"\n", path, real_path);

    return access((const char*)real_path, mask);
}

int my_releasedir(const char *path, struct fuse_file_info *fi)
{
    fprintf(log_file, "realeasedir(%s)\n", path);
    return closedir((DIR *) (uintptr_t) fi->fh);
}

int my_release(const char *path, struct fuse_file_info *fi)
{
    fprintf(log_file, "realease(%s)\n", path);
    return close(fi->fh);
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
    .chmod      = my_chmod,
    .chown      = my_chown,
    .truncate   = my_truncate,
    .utime      = my_utime,
    .fgetattr   = my_fgetattr,
    .access     = my_access,
    .ftruncate  = my_ftruncate,
    .releasedir = my_releasedir,
    .release    = my_release
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
    memset(key + strlen((char*)key), 0, 16 - strlen((char*)key));
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    root_dir = realpath(argv[argc-2], NULL);
    root_dir_len = strlen(root_dir);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;

    log_file = fopen("log.log", "w");
    if (log_file == NULL)
    {
        perror("log open");
        abort();
    }
    setvbuf(log_file, NULL, _IOLBF, 0);
    
    return fuse_main(argc, argv, &operations, NULL);
}
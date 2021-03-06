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

static int my_syscall(int ret)
{
    if (ret < 0)
        ret = -errno;

    return ret;
}

int my_getattr(const char *path, struct stat *statbuf)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "getattr(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(lstat((const char*)real_path, statbuf));
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
    fprintf(log_file, "mkdir(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(mkdir((const char*)real_path, mode));
}

int my_mknod(const char *path, mode_t mode, dev_t dev)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "mknod(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(mknod((const char*)real_path, mode, dev));
}

int my_unlink(const char *path)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "unlink(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(unlink((const char*)real_path));
}

int my_rmdir(const char *path)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "rmdir(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(rmdir((const char*)real_path));
}

int my_open(const char *path, struct fuse_file_info *fi)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "open(%s): real_path: \"%s\"\n", path, real_path);
    int fd = open((const char*)real_path, fi->flags);
	
    if (fd >= 0)
        fi->fh = fd;
    else
        return -errno;
    
    return 0;
}

int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    fprintf(log_file, "read(%s)", path);
    cipher_t encrypted[8192];

    int ret = my_syscall(pread(fi->fh, encrypted, size, offset));
    if (ret >= 0)
    {
        ret = my_decrypt_binary(encrypted, size, key, iv, buf);
    }

    return ret;
}

int my_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    fprintf(log_file, "write(%s)", path);
    //TODO: encrypt data
    cipher_t encrypted[8192];
    size = my_encrypt_binary(buf, size, key, iv, encrypted);
    return my_syscall(pwrite(fi->fh, (char*)encrypted, size, offset));
}

int my_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    fprintf(log_file, "fsync(%s)", path);
    if (datasync)
	    return my_syscall(fdatasync(fi->fh));
    else
	    return my_syscall(fsync(fi->fh));
}

int my_chmod(const char *path, mode_t mode)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "chmod(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(chmod((const char*)real_path, mode));
}

int my_chown(const char *path, uid_t uid, gid_t gid)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "chown(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(chown((const char*)real_path, uid, gid));
}

int my_truncate(const char *path, off_t newsize)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "truncate(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(truncate((const char*)real_path, newsize));
}

int my_utime(const char *path, struct utimbuf *ubuf)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "utime(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(utime((const char*)real_path, ubuf));
}

int my_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    fprintf(log_file, "fgetattr(%s)\n", path);
    return my_syscall(fstat(fi->fh, statbuf));
}

int my_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    fprintf(log_file, "ftruncate(%s)\n", path);
    return my_syscall(ftruncate(fi->fh, offset));
}

int my_access(const char *path, int mask)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "access(%s): real_path: \"%s\"\n", path, real_path);

    return my_syscall(access((const char*)real_path, mask));
}

int my_opendir(const char *path, struct fuse_file_info *fi)
{
    cipher_t real_path[PATH_MAX];
    get_real_path(path, real_path, key, iv);
    fprintf(log_file, "opendir(%s): real_path: \"%s\"\n", path, real_path);
    DIR *dp = opendir((const char*)real_path);
    if (dp == NULL)
    {
        fi->fh = -1;
	    return -errno;
    }
    
    fi->fh = (intptr_t)dp;
    
    return 0;
}


int my_releasedir(const char *path, struct fuse_file_info *fi)
{
    fprintf(log_file, "realeasedir(%s)\n", path);
    closedir((DIR *) (uintptr_t) fi->fh);

    return 0;
}

int my_release(const char *path, struct fuse_file_info *fi)
{
    fprintf(log_file, "realease(%s)\n", path);
    close(fi->fh);

    return 0;
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
    .opendir    = my_opendir,
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
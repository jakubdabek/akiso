#include "path_util.h"

#include <stdio.h>
#include <string.h>

char *root_dir;
size_t root_dir_len;
    
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <root dir>\n", argv[0]);
        return 1;
    }

    root_dir = argv[1];
    root_dir_len = strlen(root_dir);

    char *str;
    size_t size = 0;
    ssize_t ret;
    cipher_t key[] = "0123456789012345";
    cipher_t iv[] = "0123456789012345";
    while ((ret = getline(&str, &size, stdin)) != -1)
    {
        str[ret - 1] = '\0';
        cipher_t real_path[PATH_MAX];
        ssize_t ret = get_real_path(str, real_path, key, iv);
        if (ret == 0)
        {
            printf("incorrect path\n");
        }
        else
        {
            printf("encoded: \"%s\"\n", real_path);
        }
    }
}
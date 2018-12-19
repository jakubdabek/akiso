#include "path_util.h"

#include "ssl-crypt.h"

#include <string.h>
#include <limits.h>
#include <stdio.h>

extern const char *root_dir;
extern size_t root_dir_len;

ssize_t get_real_path(const char * const path, cipher_t out_buff[PATH_MAX], cipher_t key[16], cipher_t iv[16])
{
    out_buff[0] = '\0';
    if (path[0] == '\0' || path[0] != '/')
    {
        return 0;
    }
    char buffer[PATH_MAX];
    strcpy((char*)buffer, path);
    char *saveptr;
    strcat((char*)out_buff, root_dir);
    size_t current_len = root_dir_len;
    char *token = strtok_r(buffer, "/", &saveptr);
    while (token != NULL)
    {
        out_buff[current_len++] = '/';
        current_len += my_encrypt_base64(token, strlen(token), key, iv, (cipher_t*)&out_buff[current_len]);
        // printf("token: \"%s\"\n", token);
        // printf("current_len: %ld, out_buff: \"%*s\"\n", current_len, current_len, out_buff);
        token = strtok_r(NULL, "/", &saveptr);
    }

    out_buff[current_len + 1] = '\0';

    return strlen((char*)out_buff);
}

// static int main(int argc, char *argv[])
// {
//     if (argc < 2)
//     {
//         fprintf(stderr, "Usage: %s <root dir>\n", argv[0]);
//         return 1;
//     }

//     root_dir = argv[1];
//     root_dir_len = strlen(root_dir);

//     char *str;
//     size_t size = 0;
//     ssize_t ret;
//     while ((ret = getline(&str, &size, stdin)) != -1)
//     {
//         str[ret - 1] = '\0';
//         char real_path[PATH_MAX];
//         ssize_t ret = get_real_path(str, real_path, key, iv);
//         if (ret == 0)
//         {
//             printf("incorrect path\n");
//         }
//         else
//         {
//             printf("encoded: \"%s\"\n", real_path);
//         }
//     }
// }
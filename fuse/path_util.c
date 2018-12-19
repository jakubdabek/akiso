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
        if (strcmp(token, ".") == 0 || strcmp(token, "..") == 0)
        {
            strcpy((char*)&out_buff[current_len], token);
            current_len += strlen(token);
        }
        else
        {
            current_len += my_encrypt_base64(token, strlen(token), key, iv, &out_buff[current_len]);
        }
        // printf("token: \"%s\"\n", token);
        // printf("current_len: %ld, out_buff: \"%*s\"\n", current_len, current_len, out_buff);
        token = strtok_r(NULL, "/", &saveptr);
    }

    out_buff[current_len + 1] = '\0';

    return strlen((char*)out_buff);
}

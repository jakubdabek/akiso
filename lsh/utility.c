#include "utility.h"

#include <stdbool.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

const char * const whitespace = " \t\n\r";

void flush(void)
{
    char c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

char* read_line(char * const buffer, const size_t size)
{
    char *ptr = buffer;
    size_t bytes_read = 0;
    while (true)
    {
        int ret = read(STDIN_FILENO, ptr, size - bytes_read);
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            perror("read error");
            return NULL;
        }
        if (ret == 0 && ptr == buffer)
        {
            return NULL;
        }

        if (ptr[ret - 1] == '\n' || bytes_read >= size)
        {
            ptr[ret - 1] = '\0';
            break;
        }

        ptr += ret;
    }

    // if (!fgets(buffer, size, stdin))
    //     return NULL;
    // if (buffer[strlen(buffer) - 1] == '\n')
    //     buffer[strlen(buffer) - 1] = '\0';
    // else
    //     flush();

    return buffer;
}

int read_int(int * const value)
{
    char buffer[100];
    if(!read_line(buffer, 100)) 
        return 0;
    int success = sscanf(buffer, "%d", value);

    return success;
}

bool char_in_set(const char c, const char *set)
{
    for ( ; *set != '\0'; set++)
    {
        if (*set == c)
            return true;
    }

    return false;
}

const char* trim_front(const char *str, const char * const characters)
{
    while (char_in_set(*str, characters))
        str++;
    
    return str;
}

char* trim_end(char * const str, const char * const characters)
{
    size_t len = strlen(str);
    for (int i = len - 1; i > 0; i++)
    {
        if (char_in_set(str[i], characters))
        {
            str[i] = '\0';
        }
        else
        {
            return str;
        }
    }

    return str;
}

char* trim(char * const str, const char * const characters)
{
    return (char*)trim_front(trim_end(str, characters), characters);
}

char* tokenize(char * const str, const char * const delimeters, bool can_backslash_escape)
{
    size_t len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (char_in_set(str[i], delimeters) && !(i > 0 && can_backslash_escape && str[i - 1] == '\\'))
        {
            str[i] = '\0';
        }
    }
    return &str[len];
}

const char* next_token(const char *str, const char * const last)
{
    if (str == NULL)
        return NULL;

    while (str != last && *str != '\0')
        str++;

    while (str != last && *str == '\0')
        str++;

    if (str == last)
        return NULL;
    else
        return str;
}

size_t get_argc(const char * const *argv)
{
    if (argv == NULL)
        return 0;
    
    size_t count = 0;
    while (*argv++ != NULL)
        count++;

    return count;
}

int set_signal_handler(int signal, void (*callback)(int))
{
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART;
    act.sa_handler = callback;
    return sigaction(signal, &act, NULL);
}

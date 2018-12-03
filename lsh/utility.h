#pragma once

#include <stdbool.h>
#include <stddef.h>

void flush(void);
char* read_line(char *buffer, size_t size, volatile bool *canceler);
int read_int(int *value);
const char* trim_front(const char *str, const char *characters);
char* trim_end(char *str, const char *characters);
char* trim(char *str, const char *characters);
char* tokenize(char *str, const char *delimeters, bool can_backslash_escape);
const char* next_token(const char *str, const char *last);
bool char_in_set(char c, const char *set);
size_t get_argc(const char * const *argv);

int set_signal_handler(int signal, void (*callback)(int));
void set_signal_mask(int signum, bool on);

extern const char * const whitespace;

#define LINKED_LIST_ADD_LAST(type)                                  \
struct type** type##_add_last(struct type **head, struct type *arg) \
{                                                                   \
    if (head == NULL)                                               \
        return NULL;                                                \
    while (*head != NULL)                                           \
        head = &((*head)->next);                                    \
    *head = arg;                                                    \
    return &((*head)->next);                                        \
}

#define LINKED_LIST_ADD_FIRST(type)                         \
void type##_add_first(struct type **head, struct type *arg) \
{                                                           \
    if (head == NULL)                                       \
        return;                                             \
    if (*head == NULL)                                      \
    {                                                       \
        *head = arg;                                        \
    }                                                       \
    else                                                    \
    {                                                       \
        arg->next = *head;                                  \
        *head = arg;                                        \
    }                                                       \
}

#define LINKED_LIST_SIZE(type)                              \
size_t type##_list_size(const struct type *head)            \
{                                                           \
    size_t size = 0;                                        \
    while (head != NULL)                                    \
    {                                                       \
        size++;                                             \
        head = head->next;                                  \
    }                                                       \
    return size;                                            \
}

#define LINKED_LIST_POP_FIRST(type)                         \
struct type* type##_pop_first(struct type **head)           \
{                                                           \
    if (head == NULL || *head == NULL)                      \
    {                                                       \
        return NULL;                                        \
    }                                                       \
    else                                                    \
    {                                                       \
        struct type *tmp = *head;                           \
        *head = (*head)->next;                              \
        return tmp;                                         \
    }                                                       \
}

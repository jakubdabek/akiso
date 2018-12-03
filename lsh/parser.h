#pragma once

#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>

#define BUFF_SIZE 512
#define MAX_ARGS 20

enum parse_result { PR_OK = 1, PR_EXIT = 0, PR_SYNTAX_ERROR = -1, PR_ERROR_ERRNO = -2 };

const char* parse(char * buff, size_t buff_size, char ** arguments, size_t max_args, bool * bg);
int execute(const char * const command, char * const * const arguments, bool bg);
//bool interpret_line(const char *line);
enum parse_result interpret_line(const char *line);

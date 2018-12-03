#include "utility.h"
#include "parser.h"
#include "job.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

extern int current_terminal;
extern sigset_t default_mask;
extern struct termios terminal_config;

char** parse_tokens(const char * const command, size_t * const argc, const char * const delimeters, const bool allow_empty)
{
    size_t arg_count = 0;
    const size_t len = strlen(command);
    void *_tmp = malloc((MAX_ARGS + 1) * sizeof(char*) + (len + 1) * sizeof(char));
    //char *buffer = malloc(len + 1);
    char **arguments = (char**)_tmp;
    char *buffer = (char*)(((char**)_tmp) + MAX_ARGS + 1);
    //char **arguments = calloc(MAX_ARGS + 1, sizeof(*arguments));

    const char *command_current_ptr = command;
    bool escaped = false, add_arg = true;

    while (*command_current_ptr != '\0')
    {
        if (char_in_set(*command_current_ptr, delimeters))
        {
            if (escaped)
            {
                escaped = false;
            }
            else
            {
                if (allow_empty && add_arg)
                {
                    arguments[arg_count++] = buffer;
                    *buffer++ = '\0';
                }
                else if (!add_arg)
                {
                    *buffer++ = '\0';
                }
                add_arg = true;
                command_current_ptr++;
                continue;
            }
        }

        if (add_arg)
        {
            arguments[arg_count++] = buffer;
            add_arg = false;
            if (arg_count >= MAX_ARGS)
            {
                free(arguments);
                if (argc != NULL)
                    *argc = 0;
                return NULL;
            }
        }

        if (*command_current_ptr == '\\')
        {
            if (escaped)
            {
                *buffer++ = '\\';
                escaped = false;
            }
            else
            {
                escaped = true;
            }
        }
        else
        {
            if (escaped)
            {
                *buffer++ = '\\';
                *buffer++ = *command_current_ptr;
                escaped = false;
            }
            else
            {
                *buffer++ = *command_current_ptr;
            }
        }

        command_current_ptr++;
    }
    if (escaped)
    {
        *buffer++ = '\\';
        *buffer = '\0';
    }
    else if (add_arg && allow_empty)
    {
        arguments[arg_count++] = buffer;
        *buffer++ = '\0';
        if (arg_count >= MAX_ARGS)
        {
            free(arguments);
            if (argc != NULL)
                *argc = 0;
            return NULL;
        }
    }
    else
    {
        *buffer = '\0';
    }

    arguments[arg_count] = NULL;
    if (argc != NULL)
    {
        *argc = arg_count;
    }
    return arguments;
}

int get_terminal(int tty, pid_t pid)
{
    sigset_t set, oldset;
    sigemptyset(&set);
    sigaddset(&set, SIGTTOU);
    sigemptyset(&oldset);
    sigprocmask(SIG_BLOCK, &set, &oldset);
    int ret = tcsetpgrp(current_terminal, getpid());
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    return ret;
}

bool check_redirect(struct redirect * const redirect)
{
    const char *proc;
    const char *other;
    int *proc_fd;
    int *other_fd;
    if (redirect->is_out)
    {
        proc = redirect->from;
        proc_fd = &redirect->from_fd;
        other = redirect->to;
        other_fd = &redirect->to_fd;
    }
    else
    {
        proc = redirect->to;
        proc_fd = &redirect->to_fd;
        other = redirect->from;
        other_fd = &redirect->from_fd;
    }
    char *ptr;
    if (proc[0] == '\0')
    {
        *proc_fd = redirect->is_out ? 1 : 0;
    }
    else
    {
        *proc_fd = strtol(proc, &ptr, 10);
        if (*ptr != '\0')
            return false;
    }

    if (other[0] == '\0')
        return false;
    
    if (other[0] == '&')
    {
        if (other[1] == '\0')
            return false;

        *other_fd = strtol(other + 1, &ptr, 10);
        if (*ptr != '\0')
            return false;
    }
    else
    {
        *other_fd = -1;
    }

    return true;
}

struct redirect* parse_redirect(const char * const str, bool is_out)
{
    size_t side_count;
    char **redirect_sides = parse_tokens(str, &side_count, is_out ? ">" : "<", true);
    if (side_count != 2)
    {
        free(redirect_sides);
        return NULL;
    }

    struct redirect *redirect = malloc(sizeof(*redirect));
    redirect->from = strdup(redirect_sides[is_out ? 0 : 1]);
    redirect->to = strdup(redirect_sides[is_out ? 1 : 0]);
    redirect->is_out = is_out;
    redirect->next = NULL;

    free(redirect_sides);
    if (!check_redirect(redirect))
    {
        destroy_redirect(redirect);
        return NULL;
    }

    return redirect;
}

#define EMPTY_PROCESS ((struct process*)0)
#define ERROR_PROCESS ((struct process*)-1)

struct process* parse_process(const char * const command)
{
    size_t argc;
    char **tokens = parse_tokens(command, &argc, whitespace, false);
    if (argc <= 0)
    {
        free(tokens);
        return EMPTY_PROCESS;
    }
    struct process *process = malloc(sizeof(*process));
    empty_process(process);

    bool redirect_found = false;
    bool error = false;

    process->arguments = tokens;

    for (size_t i = 0; i < argc; i++)
    {
        bool is_out = false, is_in = false;
        if ((is_out = char_in_set('>', tokens[i])) || (is_in = char_in_set('<', tokens[i])))
        {
            if (i == 0)
            {
                error = true;
                break;
            }
            if (is_in && is_out)
            {
                error = true;
                break;
            }
            struct redirect *redirect = parse_redirect(tokens[i], is_out);
            tokens[i] = NULL;
            if (redirect == NULL)
            {
                error = true;
                break;
            }
            redirect_add_last(&(process->redirects), redirect);
            if (!redirect_found)
                process->argc = i;
            redirect_found = true;
        }
        else if (redirect_found)
        {
            error = true;
            break;
        }
    }

    if (!redirect_found)
        process->argc = argc;

    if (error)
    {
        destroy_process(process);
        return ERROR_PROCESS;
    }

    return process;
}

enum parse_result interpret_line(const char * const line)
{
    bool bg = false;
    char buff[BUFF_SIZE];
    strncpy(buff, trim_front(line, whitespace), BUFF_SIZE);
    trim_end(buff, whitespace);
    size_t len = strlen(buff);
    if (buff[len - 1] == '&')
    {
        bg = true;
        buff[len - 1] = '\0';
    }

    size_t pipeline_size;
    char **processes = parse_tokens(buff, &pipeline_size, "|", true);
    if (pipeline_size == 0)
    {
        if (bg)
            return PR_SYNTAX_ERROR;
        else
            return PR_OK;
    }

    struct job *job = malloc(sizeof(*job));
    empty_job(job);
    job->fg = !bg;
    for (size_t i = 0; i < pipeline_size; i++)
    {
        struct process *process = parse_process(processes[i]);
        if (process == ERROR_PROCESS)
        {
            destroy_job(job);
            return PR_SYNTAX_ERROR;
        }
        if (process == EMPTY_PROCESS)
        {
            destroy_job(job);
            return PR_OK;
        }

        process_add_last(&(job->pipeline), process);
    }

    job->pipeline_size = pipeline_size;

    //print_job(job);

    if (strcmp(job->pipeline->arguments[0], "exit") == 0)
    {
        destroy_job(job);
        return PR_EXIT;
    }
    if (strcmp(job->pipeline->arguments[0], "cd") == 0)
    {
        if (job->pipeline->next != NULL)
        {
            destroy_job(job);
            return PR_SYNTAX_ERROR;
        }

        if (job->pipeline->argc > 2)
        {
            destroy_job(job);
            return PR_SYNTAX_ERROR;
        }
        else if (job->pipeline->argc == 1)
        {
            int ret = chdir(getenv("HOME"));
            if (ret == -1)
            {
                return PR_ERROR_ERRNO;
            }
        }
        else
        {
            int ret = chdir(job->pipeline->arguments[1]);
            if (ret == -1)
            {
                destroy_job(job);
                return PR_ERROR_ERRNO;
            }
        }
    }
    else
    {
        start_job(job);
        if (get_terminal(current_terminal, getpgrp()) == -1)
        {
            perror("tty");
            exit(1);
        }
        if (tcsetattr(current_terminal, TCSADRAIN, &terminal_config) == -1)
        {
            perror("tty");
            exit(1);
        }
        tcflush(current_terminal, TCIFLUSH);
    }

    return PR_OK;
}


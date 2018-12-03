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

void reset_job_control_handlers()
{
    set_signal_handler(SIGTSTP, SIG_DFL);
    set_signal_handler(SIGTTIN, SIG_DFL);
    set_signal_handler(SIGTTOU, SIG_DFL);
    set_signal_handler(SIGINT,  SIG_DFL);
    set_signal_handler(SIGQUIT, SIG_DFL);
    set_signal_handler(SIGCHLD, SIG_DFL);
}

int do_redirects(struct redirect *redirect)
{
    while (redirect != NULL)
    {
        if (redirect->is_out)
        {
            if (redirect->to_fd == -1)
            {
                redirect->to_fd = open(redirect->to, O_WRONLY | O_CREAT);
                if (redirect->to_fd == -1)
                    return -1;
            }
            if (dup2(redirect->to_fd, redirect->from_fd) == -1)
                return -1;
            close(redirect->to_fd);
        }
        else
        {
            if (redirect->from_fd == -1)
            {
                redirect->from_fd = open(redirect->from, O_RDONLY);
                if (redirect->from_fd == -1)
                    return -1;
            }
            if (dup2(redirect->from_fd, redirect->to_fd) == -1)
                return -1;
            close(redirect->from_fd);
        }

        redirect = redirect->next;
    }

    return 0;
}

int link_pipes(int in_fd, int out_fd)
{
    int in_ret = 0, out_ret = 0;
    if (in_fd != -2)
        in_ret = dup2(in_fd, STDIN_FILENO);
    if (out_fd != -2)
        out_ret = dup2(out_fd, STDOUT_FILENO);
    if (in_ret == -1 || out_ret == -1)
        return -1;
    return 0;
}

int close_pipe(int fds[2], int which)
{
    int ret = 0;
    if (which & 1 && fds[0] > 0)
    {
        if (close(fds[0]) == -1)
            ret = -1;
        else
            fds[0] = -1;
    }
    if (which & 2 && fds[1] > 0)
    {
        if (close(fds[1]) == -1)
            ret = -1;
        else
            fds[1] = -1;
    }

    return ret;
}

int create_pipe(int fds[2], bool close_on_exec)
{
    if (pipe(fds) == -1)
        return -1;
    if (close_on_exec &&
        (fcntl(fds[0], F_SETFD, FD_CLOEXEC) == -1 ||
         fcntl(fds[1], F_SETFD, FD_CLOEXEC) == -1))
    {
        close_pipe(fds, 3);
        return -1;
    }

    return 0;
}

int read_error(const int fd, int * const err, char * const buff, size_t buffsize)
{
    while (true)
    {
        int ret = read(fd, err, sizeof(*err));
        if (ret == 0)
            return 0;
        if ((ret == -1 && errno != EINTR) || ret != sizeof(*err))
            return -1;

        ret += read(fd, buff, buffsize);
        if (ret >= 0)
        {
            buff[ret - sizeof(*err)] = '\0';
            return ret;
        }
    }
}

int write_error(const int fd, const char * const message, bool fatal)
{
    int ret = write(fd, &errno, sizeof(errno));
    ret += write(fd, message, strlen(message));
    if (fatal)
        _exit(1);
    
    return ret;
}


int start_job(struct job *job)
{
    if (job->fg)
    {
        tcgetattr(current_terminal, &terminal_config);
    }
    if (job == NULL)
        return -1;
    int control_pipe[2] = { -1, -1 };
    if (create_pipe(control_pipe, /*close-on-exec*/true) == -1)
    {
        return -1;
    }

    pid_t pid;
    struct process *current_process = job->pipeline;
    bool first = true, last = false;
    int prevfd = -2;
    int pipefds[2] = { -1, -1 };
    int error_pipe[2] = { -1, -1 };
    char error_buff[128];
    int first_error_pipe_read = -1;
    int child_errno = 0;


    while (current_process != NULL)
    {
        if (current_process->next == NULL)
            last = true;
        if (!last)
        {
            if (create_pipe(pipefds, /*close-on-exec*/true) == -1)
            {
                goto error_label;
            }
        }
        if (create_pipe(error_pipe, /*close-on-exec*/true) == -1)
        {
            goto error_label;
        }
        switch (pid = fork())
        {
        case -1:
            goto error_label;
        case 0:
            //child
            reset_job_control_handlers();
            sigprocmask(SIG_SETMASK, &default_mask, NULL);
            errno = 0;
            if (link_pipes(prevfd, last ? STDOUT_FILENO : pipefds[1]) == -1 || do_redirects(current_process->redirects) == -1)
            {
                write_error(error_pipe[1], "pipe or redirect", /*fatal*/true);
            }
            close_pipe(&prevfd, 1);
            if (!last)
                close_pipe(pipefds, 2);
            setuid(getuid());
            setgid(getgid());
            if (first)
            {
                setpgid(0, 0);
                
                if (job->fg)
                {
                    errno = 0;
                    if (get_terminal(current_terminal, getpgrp()) == -1)
                    {
                        write_error(error_pipe[1], "couldn't get terminal", /*fatal*/true);
                    }
                }

                char c;
                if (close_pipe(control_pipe, 2) == -1)
                {
                    write_error(error_pipe[1], "pipe", /*fatal*/true);
                }
                //printf("reading control\n");
                while (read(control_pipe[0], &c, 1) == -1 && errno == EINTR) {}
                //printf("read control\n");
                close(control_pipe[0]);
            }
            else
            {
                setpgid(0, job->pgid);
                close(control_pipe[0]);
            }
            execvp(current_process->arguments[0], current_process->arguments);
            write_error(error_pipe[1], current_process->arguments[0], /*fatal*/true);
            break;
        default:
            close_pipe(&prevfd, 1);
            if (!last)
            {
                close_pipe(pipefds, 2);
                prevfd = pipefds[0];
            }
            if (first)
            {
                job->pgid = pid;
            }
            setpgid(pid, job->pgid);
            if (close_pipe(error_pipe, 2) == -1)
                goto error_label;
            if (!first)
            {
                if (read_error(error_pipe[0], &child_errno, error_buff, 128) != 0)
                {
                    goto error_label;
                }
            }
            else
            {
                first_error_pipe_read = error_pipe[0];
            }
        }
        current_process = current_process->next;
        first = false;
    }

    close_pipe(control_pipe, 3);
    close_pipe(pipefds, 3);
    if (read_error(first_error_pipe_read, &child_errno, error_buff, 128) != 0)
        goto error_label;
    close_pipe(error_pipe, 3);

    if (job->fg)
    {
        int status;
        bool falling_apart = false;
        bool stopped = false;
        size_t stopped_process_count = 0;
        while (waitpid(-job->pgid, &status, WUNTRACED) > 0)
        {
            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                falling_apart = true;
                if (stopped)
                {
                    killpg(job->pgid, SIGTERM);
                    killpg(job->pgid, SIGCONT);
                    stopped_process_count = 0;
                }
            }
            else if (WIFSTOPPED(status))
            {
                stopped = true;
                if (falling_apart)
                {
                    killpg(job->pgid, SIGTERM);
                    killpg(job->pgid, SIGCONT);
                    stopped_process_count = 0;
                }
                else
                {
                    stopped_process_count++;
                }
            }

            if (stopped_process_count == job->pipeline_size)
            {
                job->fg = false;
                job_add_first(&current_job, job);
                break;
            }
        }
    }
    else
    {
        job_add_first(&current_job, job);
        job_add_last(&pending_jobs, job);
        //pending_jobs++;
    }

    return 0;

error_label:;
    //TODO: cleanup
    close_pipe(control_pipe, 3);
    close_pipe(pipefds, 3);
    close_pipe(error_pipe, 3);
    if (child_errno != 0)
    {
        int olderr = errno;
        errno = child_errno;
        perror(error_buff);
        errno = olderr;
    }
    //printf("ERROR: %s\n", error_buff);
    return -1;
}

int execute(const char * const command, char * const * const arguments, bool bg)
{
    pid_t child_pid = fork();
    if (child_pid == -1)
    {
        perror("Error while forking");
        return -1;
    }

    if (child_pid == 0)
    {
        setuid(getuid());
        setgid(getgid());
        if (setpgid(0, 0) == -1)
        {
            perror("Error assigning group");
            exit(1);
        }

        set_signal_handler(SIGTSTP, SIG_DFL);
        set_signal_handler(SIGTTIN, SIG_DFL);
        set_signal_handler(SIGTTOU, SIG_DFL);
        set_signal_handler(SIGINT,  SIG_DFL);
        set_signal_handler(SIGQUIT, SIG_DFL);
        set_signal_handler(SIGCHLD, SIG_DFL);

        tcsetpgrp(current_terminal, getpgid(getpid()));
        close(current_terminal);

        if (execvp(command, arguments) == -1)
        {
            char sprintf_buffer[BUFF_SIZE];
            switch (errno)
            {
            case ENOENT:
                snprintf(sprintf_buffer, BUFF_SIZE, "Command '%s' not found", command);
                perror(sprintf_buffer);
                exit(1);
            default:
                snprintf(sprintf_buffer, BUFF_SIZE, "Error starting command '%s'", command);
                perror(sprintf_buffer);
                exit(1);
            }
        }
    }
    else if (!bg)
    {
        int status;
        setpgid(child_pid, 0);
        tcsetpgrp(current_terminal, getpgid(child_pid));
        struct job *new_job = malloc(sizeof(*current_job));
        new_job->pgid = child_pid;
        new_job->fg = false;
        job_add_first(&current_job, new_job);
        waitpid(child_pid, &status, 0);
        // SIGTTIN SIGTTOU etc.
        tcsetpgrp(current_terminal, getpgid(getpid()));
        close(current_terminal);
        if (WIFEXITED(status))
        {
            int ret = WEXITSTATUS(status);
            return ret;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        struct job *new_job = malloc(sizeof(*current_job));
        new_job->pgid = child_pid;
        new_job->fg = false;
        job_add_first(&current_job, new_job);
        //pending_jobs++;
    }

    return 0;
}

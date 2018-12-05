#include "execute.h"
#include "parser.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <stdbool.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#define BUFF_SIZE 512

extern int current_terminal;
extern sigset_t default_mask;
extern struct termios terminal_config;
extern jmp_buf env;

const char * const builtins[] = 
{
    "cd",
    "jobs",
    "fg",
    "bg",
    "exit"
};

const builtin_command builtin_commands[] =
{
    do_cd,
    do_jobs,
    do_fg,
    do_bg,
    do_exit
};

size_t builtins_count = sizeof(builtins) / sizeof(*builtins);

static int get_terminal(int tty, pid_t pid)
{
    sigset_t set, oldset;
    sigemptyset(&set);
    sigaddset(&set, SIGTTOU);
    sigemptyset(&oldset);
    sigprocmask(SIG_BLOCK, &set, &oldset);
    int ret = tcsetpgrp(current_terminal, pid);
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    return ret;
}

static void reset_job_control_handlers()
{
    set_signal_handler(SIGTSTP, SIG_DFL);
    set_signal_handler(SIGTTIN, SIG_DFL);
    set_signal_handler(SIGTTOU, SIG_DFL);
    set_signal_handler(SIGINT,  SIG_DFL);
    set_signal_handler(SIGQUIT, SIG_DFL);
    set_signal_handler(SIGCHLD, SIG_DFL);
}

static int do_redirects(struct redirect *redirect)
{
    while (redirect != NULL)
    {
        if (redirect->right_fd == -1)
        {
            if (redirect->is_out)
                redirect->right_fd = open(redirect->right, O_WRONLY | O_CREAT, 0766);
            else
                redirect->right_fd = open(redirect->right, O_RDONLY);

            if (redirect->right_fd == -1)
                return -1;
        }
        if (dup2(redirect->right_fd, redirect->left_fd) == -1)
            return -1;
        close(redirect->right_fd);

        redirect = redirect->next;
    }

    return 0;
}

static int link_pipes(int in_fd, int out_fd)
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

static int close_pipe(int fds[2], int which)
{
    int ret = 0;
    if ((which & 1) && fds[0] > 0)
    {
        if (close(fds[0]) == -1)
            ret = -1;
        else
            fds[0] = -1;
    }
    if ((which & 2) && fds[1] > 0)
    {
        if (close(fds[1]) == -1)
            ret = -1;
        else
            fds[1] = -1;
    }

    return ret;
}

static int create_pipe(int fds[2], bool close_on_exec)
{
    if (pipe(fds) == -1)
        return -1;

    if (close_on_exec)
    {
        int flags0 = fcntl(fds[0], F_GETFD);
        if (flags0 == -1)
            goto ret_error;
        int flags1 = fcntl(fds[1], F_GETFD);
        if (flags1 == -1)
            goto ret_error;
        
        flags0 |= FD_CLOEXEC;
        flags1 |= FD_CLOEXEC;

        if (fcntl(fds[0], F_SETFD, flags0) == -1)
            goto ret_error;
        if (fcntl(fds[1], F_SETFD, flags1) == -1)
            goto ret_error;
    }

    return 0;

ret_error:;
    close_pipe(fds, 3);
    return -1;
}

static int read_error(const int fd, int * const err, char * const buff, size_t buffsize)
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

static int write_error(const int fd, const char * const message, bool fatal)
{
    int ret = write(fd, &errno, sizeof(errno));
    ret += write(fd, message, strlen(message));
    if (fatal)
        _exit(1);
    
    return ret;
}

static int start_job_internal(struct job *job)
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
                goto error_label;
        }
        if (create_pipe(error_pipe, /*close-on-exec*/true) == -1)
            goto error_label;

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
            //parent
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
                close_pipe(error_pipe, 1);
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
    close_pipe(&first_error_pipe_read, 1);
    close_pipe(error_pipe, 3);

    return 0;

error_label:;
    //TODO: cleanup
    if (job->pgid > 0)
    {
        killpg(job->pgid, SIGTERM);
        killpg(job->pgid, SIGCONT);
    }
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
    return -1;
}

static int wait_for_fg_job(struct job *job)
{
    set_signal_mask(SIGCHLD, true);
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
            job->stopped = true;
            job_handle_add_first(&current_job, make_job_handle(job));
            job_handle_add_last(&pending_jobs, make_job_handle(job));
            break;
        }
    }
    set_signal_mask(SIGCHLD, false);

    return 0;
}

static int launch_job(struct job *job)
{
    set_signal_mask(SIGCHLD, true);
    if (job->fg)
    {
        wait_for_fg_job(job);
    }
    else
    {
        job_handle_add_first(&current_job, make_job_handle(job));
        job_handle_add_last(&pending_jobs, make_job_handle(job));
    }
    set_signal_mask(SIGCHLD, false);
    if (get_terminal(current_terminal, getpgrp()) == -1)
    {
        perror("tty");
        longjmp(env, 1);
    }
    if (tcsetattr(current_terminal, TCSADRAIN, &terminal_config) == -1)
    {
        perror("tty");
        longjmp(env, 1);
    }
    tcflush(current_terminal, TCIFLUSH);

    return 0;
}

int start_job(struct job *job)
{
    int ret = start_job_internal(job);
    if (ret != -1)
        launch_job(job);

    return ret;
}


enum parse_result do_cd(const char * const * const args, const int argc)
{
    if (argc > 2)
    {
        return PR_SYNTAX_ERROR;
    }
    else if (argc == 1)
    {
        int ret = chdir(getenv("HOME"));
        if (ret == -1)
        {
            return PR_ERROR_ERRNO;
        }
    }
    else
    {
        int ret = chdir(args[1]);
        if (ret == -1)
        {
            return PR_ERROR_ERRNO;
        }
    }

    return PR_OK;
}

enum parse_result do_jobs(const char * const * const args, const int argc)
{
    struct job_handle *ptr = current_job;
    int i = 1;
    while (ptr != NULL)
    {
        print_job(i++, ptr->job, false);
        ptr = ptr->next;
    }
    return PR_OK;
}

struct job_handle* get_job_handle(const char * const * const args, const int argc)
{
    if (argc > 2)
    {
        return (struct job_handle*)-1;
    }
    int index = 1;
    if (argc > 1)
    {
        char *ptr;
        index = strtol(args[1], &ptr, 10);
        if (args[1][0] == '\0' || *ptr != '\0')
        {
            return (struct job_handle*)-1;
        }
    }
    struct job_handle *handle = current_job;
    int i = 0;
    while (handle != NULL)
    {
        if (++i == index)
            break;
        handle = handle->next;
    }
    if (i != index)
        return NULL;
    
    handle = remove_job_handle(&current_job, handle->job->pgid);
    if (handle == NULL)
        return (struct job_handle*)-999;

    return handle;
}

enum parse_result do_fg(const char * const * const args, const int argc)
{
    struct job_handle *handle = get_job_handle(args, argc);
    if (handle == (struct job_handle*)-1)
        return PR_SYNTAX_ERROR;
    else if (handle == NULL)
        return PR_OTHER_ERROR;
    
    if (get_terminal(current_terminal, handle->job->pgid) == -1)
    {
        job_handle_add_last(&current_job, handle);
        return PR_ERROR_ERRNO;
    }
    killpg(handle->job->pgid, SIGCONT);
    handle->job->stopped = false;
    handle->job->fg = true;
    launch_job(handle->job);
    free(handle);
    
    return PR_OK;
}

enum parse_result do_bg(const char * const * const args, const int argc)
{
    struct job_handle *handle = get_job_handle(args, argc);
    if (handle == (struct job_handle*)-1)
        return PR_SYNTAX_ERROR;
    else if (handle == NULL)
        return PR_OTHER_ERROR;
    
    killpg(handle->job->pgid, SIGCONT);
    handle->job->stopped = false;
    job_handle_add_last(&current_job, handle);
    job_handle_add_last(&pending_jobs, make_job_handle(handle->job));
    
    return PR_OK;
}

enum parse_result do_exit(const char * const * const args, const int argc) 
{
    return PR_EXIT;
}


#include "execute.h"

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

static int get_terminal(int tty, pid_t pid)
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
                redirect->right_fd = open(redirect->right, O_WRONLY | O_CREAT);
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

int start_job(struct job *job)
{
    int ret = start_job_internal(job);
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

    return ret;
}


//deprecated
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

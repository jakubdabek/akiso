#include "utility.h"
#include "parser.h"
#include "execute.h"
#include "job.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

jmp_buf env;
int current_terminal;
struct termios terminal_config;
bool sigint_received = false;
sigset_t default_mask;


bool print_pending_jobs()
{
    struct job *ptr;
    bool printed = false;
    while ((ptr = remove_job(&pending_jobs, 0)) != NULL)
    {
        if (ptr->stopped)
            printf("job: %8ld (Stopped)\n", (long)ptr->pgid);
        else
            printf("job: %8ld (Running)\n", (long)ptr->pgid);
        printed = true;
    }
    while ((ptr = remove_job(&pending_removed_jobs, 0)) != NULL)
    {
        printf("job: %8ld (Done)\n", (long)ptr->pgid);
        free(ptr);
        printed = true;
    }

    return printed;
}

void print_jobs()
{
    struct job *ptr = current_job;
    while (ptr != NULL)
    {

    }
}

void print_prompt()
{
    if (sigint_received)
    {
        sigint_received = false;
        printf("\n");
    }
    print_pending_jobs();
    char buff[BUFF_SIZE];
    printf("%s$ ", getcwd(buff, BUFF_SIZE));
    fflush(stdout);
}

void handle_chld(int _, siginfo_t *info, void* __)
{
    pid_t pid;
    int status;

    struct job *job = current_job;
    while (job != NULL)
    {
        while (true)
        {
            pid = waitpid(-job->pgid, &status, WNOHANG | WUNTRACED);
            if (pid > 0)
            {
                if (WIFEXITED(status) || WIFSIGNALED(status))
                {
                    job->pipeline_size--;
                    job->falling_apart = true;
                    killpg(job->pgid, SIGCONT);
                }
                else if (WIFSTOPPED(status))
                {
                    killpg(job->pgid, SIGSTOP);
                }
            }
            else
            {
                break;
            }
        }

        pid_t pgid = job->pgid;
        int pipeline_size = job->pipeline_size;
        job = job->next;

        if (pipeline_size == 0)
        {
            struct job *removed = remove_job(&current_job, pgid);
            if (removed != NULL)
            {
                job_add_last(&pending_removed_jobs, removed);
            }
        }
    }

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        struct job *removed = remove_job(&current_job, pid);
        job_add_first(&pending_removed_jobs, removed);
    }
}

void handle_int(int signum)
{
    // if (current_job != NULL && current_job->fg)
    // {
    //     killpg(current_job->pgid, this);
    //     killpg(current_job->pgid, SIGCONT);
    // }
    // printf("\n");
    // print_prompt();
    // fflush(stdout);
    sigint_received = true;
    tcflush(current_terminal, TCIFLUSH);
}

int copy_to_high_fd(int fd, int maxfd)
{
    int newfd;
    for (newfd = maxfd - 1; newfd > 10; newfd--)
    {
        int _;
        if (fcntl(newfd, F_GETFD, &_) == -1)
            break;
    }

    if (dup2(fd, newfd) == -1)
    {
        return fd;
    }

    return newfd;
}

void prepare_handlers()
{
    sigemptyset(&default_mask);
    sigprocmask(SIG_BLOCK, NULL, &default_mask);
    sigdelset(&default_mask, SIGCHLD);

    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    set_signal_mask(SIGINT, true);
    //SIGCHLD
    {
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGINT);
        act.sa_flags = SA_RESTART | SA_SIGINFO;
        act.sa_sigaction = handle_chld;
        //act.sa_handler = SIG_DFL;
        int ret = sigaction(SIGCHLD, &act, NULL);
        if (ret == -1)
        {
            perror("Error while installing handler");
            exit(1);
        }
    }

    //SIGINT
    {
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        //act.sa_flags = SA_RESTART;
        act.sa_flags = 0;
        act.sa_handler = handle_int;
        int ret = sigaction(SIGINT, &act, NULL);
        if (ret == -1)
        {
            perror("Error while installing handler");
            exit(1);
        }
    }
}

// find / 2>/dev/null | wc -l & 
// cat aa 2>&1 >file.txt| grep -v wow\ 123 | xd >/dev/null
void start()
{
    if (!isatty(STDIN_FILENO))
    {
        fprintf(stderr, "Program was not run in an interactive session\n");
        exit(1);
    }
    current_terminal = copy_to_high_fd(STDIN_FILENO, 250);
    fcntl(current_terminal, F_SETFD, FD_CLOEXEC);
    prepare_handlers();
    tcgetattr(current_terminal, &terminal_config);

    char buff[BUFF_SIZE];
    while (true)
    {
        print_prompt();
        if (read_line(buff, BUFF_SIZE, &sigint_received) == NULL)
        {
            if (sigint_received)
                continue;
            else
                break;
        }

        // printf("%s\n", buff);
        // size_t piped_num;
        // char **piped = parse_tokens(buff, &piped_num, "|", true);
        // for (size_t i = 0; i < piped_num; i++)
        // {
        //     printf("piped[%ld]=\"%s\"\n", i, piped[i]);
        //     size_t argc;
        //     char **args = parse_tokens(piped[i], &argc, " \t\r\n", false);
        //     for (size_t j = 0; j < argc; j++)
        //     {
        //         printf("  args[%ld]=\"%s\"\n", j, args[j]);
        //     }
        //     free(args);
        // }
        switch (interpret_line(buff))
        {
        case PR_SYNTAX_ERROR:
            fprintf(stderr, "\"%s\" : syntax error\n", buff);
            break;
        case PR_ERROR_ERRNO:
            perror(buff);
            break;
        case PR_OK:
            break;
        case PR_EXIT:
            return;
        }

        // free(piped);
    }
}

void cleanup()
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    struct job *ptr = current_job;
    while (ptr != NULL)
    {
        printf("killing %d\n", ptr->pgid);
        if (killpg(ptr->pgid, SIGHUP) != -1)
        {
            killpg(ptr->pgid, SIGCONT);
            remove_job(&current_job, ptr->pgid);
        }
        ptr = ptr->next;
    }

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, 0)) > 0) 
    {
        printf("wait for %d complete\n", pid);
    }

    if (!print_pending_jobs())
    {
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    if (setjmp(env) == 0)
    {
        start();
    }
    cleanup();
}

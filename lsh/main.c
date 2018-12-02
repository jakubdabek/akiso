#include "utility.h"
#include "parser.h"
#include "job.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


bool print_jobs()
{
    struct job *ptr = current_job;
    bool printed = false;
    while (pending_jobs > 0)
    {
        printf("[new job: %d]\n", ptr->pgid);
        pending_jobs--;
        ptr = ptr->next;
        printed = true;
    }
    while (pending_removed_jobs != NULL)
    {
        struct job *tmp = remove_job(&pending_removed_jobs, 0);
        printf("[job ended: %d]\n", tmp->pgid);
        free(tmp);
        printed = true;
    }

    return printed;
}

void print_prompt()
{
    print_jobs();
    char buff[BUFF_SIZE];
    printf("%s$ ", getcwd(buff, BUFF_SIZE));
    fflush(stdout);
}

void handle_tstp(int _)
{
    // TODO: stop foreground job
}


void handle_chld(int _, siginfo_t *info, void* __)
{
    pid_t pid;
    int status;

    struct job *ptr = current_job;
    while (ptr != NULL)
    {
        pid_t pgid = ptr->pgid;
        do
        {
            pid = waitpid(-pgid, &status, WNOHANG);
        }
        while (pid > 0);

        ptr = ptr->next;

        if (kill(-pgid, 0) == -1)
        {
            struct job *removed = remove_job(&current_job, pgid);
            if (removed != NULL)
            {
                job_add_first(&pending_removed_jobs, removed);
            }
        }
    }

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        struct job *removed = remove_job(&current_job, pid);
        job_add_first(&pending_removed_jobs, removed);
    }
}

void handle_int(int this)
{
    if (current_job != NULL && current_job->fg)
    {
        killpg(current_job->pgid, this);
        killpg(current_job->pgid, SIGCONT);
    }
    printf("\n");
    print_prompt();
    fflush(stdout);
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

sigset_t default_mask;

void prepare_handlers()
{
    sigemptyset(&default_mask);
    sigprocmask(SIG_BLOCK, NULL, &default_mask);
    sigdelset(&default_mask, SIGCHLD);

    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    //SIGCHLD
    {
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        act.sa_flags = SA_RESTART | SA_SIGINFO;
        //act.sa_sigaction = handle_chld;
        act.sa_handler = SIG_DFL;
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
        act.sa_flags = SA_RESTART;
        act.sa_handler = handle_int;
        int ret = sigaction(SIGINT, &act, NULL);
        if (ret == -1)
        {
            perror("Error while installing handler");
            exit(1);
        }
    }
}

char** parse_tokens(const char *command, size_t *argc, const char *delimeters, bool allow_empty);

int current_terminal;

// find / 2> /dev/null | wc -l & 
// cat aa 2>&1 >file.txt| grep -v wow\ 123 | xd >/dev/null
void start()
{
    if (!isatty(STDIN_FILENO))
    {
        fprintf(stderr, "Program was not run in an interactive session\n");
        exit(1);
    }
    current_terminal = copy_to_high_fd(STDIN_FILENO, 250);
    prepare_handlers();

    char buff[BUFF_SIZE];
    while (true)
    {
        print_prompt();
        if (read_line(buff, BUFF_SIZE) == NULL)
            break;

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
        if (!interpret_line(buff))
            break;

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
        if (kill(-ptr->pgid, SIGHUP) != -1)
        {
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

    if (!print_jobs())
    {
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    start();
    cleanup();
}

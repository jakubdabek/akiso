#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/wait.h>

void fatal(const char *message)
{
    perror(message);
    exit(1);
}


ssize_t read_line(int fd, char *buffer, size_t n)
{
    if (buffer == NULL)
        return -1;
    ssize_t total = 0;
    for (;;) {
        char ch;
        int ret = read(fd, &ch, 1);
        if (ret == -1)
        {
            if (errno == EINTR)
                return -1;
            else
                return -1;
        }
        else if (ret == 0)
        {
            if (total == 0)
                return 0;
            else
                break;
        }
        else
        {
            if (total < n - 1)
            {
                total++;
                *buffer++ = ch;
            }
            if (ch == '\n')
                break;
        }
    }
    *buffer = '\0';
    return total;
}

void handler(int _) {}

int main(int argc, char *argv[])
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        fatal("socket");
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    int port = 8080;
    if (argc >=  2)
    {
        port = atoi(argv[1]);
        if (port <= 5000)
        {
            printf("wrong port\n");
            exit(1);
        }
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(0x7f000001);
    serv_addr.sin_port = htons(port);
    if (connect(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        fatal("connect");

    struct sigaction act;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    act.sa_handler = handler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
        fatal("sigaction");
    
    struct itimerval timer_config = 
    {
        .it_interval = { 0, 0 },
        .it_value = { 2, 0 }
    };
    struct itimerval resetter = { {0, 0}, {0, 0} };
    if (fork() == 0)
    {
        char buff[1024];
        int ret;
        setitimer(ITIMER_REAL, &timer_config, NULL);
        while ((ret = read(server_fd, buff, 10)) > 0 || errno == EINTR)
        {
            setitimer(ITIMER_REAL, &resetter, NULL);
            if (ret > 0)
                write(STDOUT_FILENO, buff, ret);
        }
    }
    else
    {
        char buff[1024];
        int ret;
        while ((ret = read_line(STDIN_FILENO, buff, 1024)) > 0)
        {
            write(server_fd, buff, ret);
            if (waitpid(-1, NULL, WNOHANG) > 0)
                return 0;
        }
    }
}
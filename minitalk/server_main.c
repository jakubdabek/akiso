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

void fatal(const char *message)
{
    perror(message);
    exit(1);
}

void handler(int signum) {}

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
                continue;
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

static const int FD_UNASSIGNED = 0;
static const int FD_AWAITING = -1;
struct user
{
    int fd;
    const char *name;
    int to;
} logged_in[16];

static void remove_user(struct user *user)
{
    close(user->fd);
    for (int i = 0; i < 16; i++)
    {
        if (logged_in[i].to == user->fd)
            logged_in[i].to = FD_UNASSIGNED;
    }
    user->fd = 0;
    free((void*)user->name);
    user->name = NULL;
    user->to = FD_UNASSIGNED;
}

int main(int argc, char *argv[])
{
    memset(logged_in, 0, sizeof(logged_in));

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
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        fatal("bind");
    if (listen(server_fd, 16) < 0)
        fatal("listen");

    struct sigaction act;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    act.sa_handler = handler;
    if (sigaction(SIGALRM, &act, NULL) == -1)
        fatal("sigaction");
    struct itimerval timer_config = 
    {
        .it_interval = { 0, 0 },
        .it_value = { 2, 00000 }
    };

    struct itimerval resetter = { {0, 0}, {0, 0} };
    while (1)
    {
        setitimer(ITIMER_REAL, &timer_config, NULL);
        int peer = accept(server_fd, NULL, NULL);
        setitimer(ITIMER_REAL, &resetter, NULL);
        fd_set readable, writable;
        FD_ZERO(&readable);
        FD_ZERO(&writable);
        if (peer == -1)
        {
            if (errno != EINTR)
            {
                fatal("accept");
            }
        }
        else
        {
            for (int i = 0; i < 16; i++)
            {
                if (logged_in[i].fd == 0)
                {
                    logged_in[i].fd = peer;
                    fprintf(stderr, "Accepted %d\n", peer);
                    write(peer, "Enter login\n", 12);
                    break;
                }
            }
        }

        int max_fd = 0;
        for (int i = 0; i < 16; i++)
        {
            struct user *user = &logged_in[i];
            if (user->fd != 0)
            {
                FD_SET(user->fd, &readable);
                if (user->name != NULL)
                {
                    if (user->to > 0)
                    {
                        FD_SET(user->to, &writable);
                        if (user->to > max_fd)
                            max_fd = user->to;
                    }
                    else if (user->to != FD_AWAITING)
                    {
                        FD_SET(user->fd, &writable);
                        user->to = FD_AWAITING;
                    }
                }
            }
        }

        struct timeval timeout = { .tv_sec = 2, .tv_usec = 200 };
        // for (int i = 0; i < 16; i++)
        // {
        //     struct user *user = &logged_in[i];
        //     if (user->fd != 0 && FD_ISSET(user->fd, &readable))
        //     {
        //         char buff[1000];
        //         read_line(user->fd, buff, 1000);
        //         fprintf(stderr, "ehhhh: \"%s\"\n", buff);
        //     }
        // }
        int select_ret = select(20, &readable, &writable, NULL, &timeout);
        fprintf(stderr, "Select returned %d\n", select_ret);
        if (select_ret > 0)
        {
            for (int i = 0; i < 16; i++)
            {
                struct user *user = &logged_in[i];
                if (user->fd != 0)
                {
                    if (FD_ISSET(user->fd, &readable))
                    {
                        char buff[256];
                        int ret = read_line(user->fd, buff, 256);
                        if (ret <= 0)
                        {
                            if (ret < 0)
                                fprintf(stderr, "Error during read from %d\n", user->fd);
                            else
                                fprintf(stderr, "Connection with %d closed\n", user->fd);
                            remove_user(user);
                        }
                        else
                        {
                            if (ret < 256)
                                buff[ret - 1] = '\0';
                            fprintf(stderr, "Read from %d: \"%s\"\n", user->fd, buff);
                            if (user->name == NULL)
                            {
                                user->name = strdup(buff);
                            }
                            else if (user->to == FD_AWAITING)
                            {
                                char *ptr;
                                int new_to = strtol(buff, &ptr, 10);
                                if (*ptr != '\0' || new_to < 0 || new_to >= 16)
                                {
                                    fprintf(stderr, "Invalid target selection from %d: \"%s\"\n", user->fd, buff);
                                    remove_user(user);
                                }
                                else
                                {
                                    struct user *target = &logged_in[new_to];
                                    if (target->fd == 0 || target->name == NULL)
                                    {
                                        fprintf(stderr, "Nonexistent target selection from %d: \"%s\"\n", user->fd, buff);
                                        remove_user(user);
                                    }
                                    else
                                    {
                                        user->to = target->fd;
                                    }
                                }
                            }
                            else if (user->to > 0)
                            {
                                if (FD_ISSET(user->to, &writable))
                                {
                                    write(user->to, buff, ret);
                                    FD_CLR(user->to, &writable);
                                }
                            }
                        }
                    }
                }
            }
            for (int i = 0; i < 16; i++)
            {
                struct user *user = &logged_in[i];
                if (FD_ISSET(user->fd, &writable))
                {
                    char buff[512];
                    ssize_t written = 0;
                    written += snprintf(buff + written, 512 - written, "Select user:\n");
                    int count = 0;
                    for (int j = 0; j < 16; j++)
                    {
                        if (i == j)
                            continue;
                        struct user *other_user = &logged_in[j];
                        if (other_user->fd != 0 && other_user->name != NULL)
                        {
                            written += snprintf(buff + written, 512 - written, "%3d - %s\n", j, other_user->name);
                            count++;
                        }
                    }
                    fprintf(stderr, "Choice given: \"%s\"\n", buff);
                    if (count > 0)
                        write(user->fd, buff, written);
                }
            }
        }
        else if (select_ret < 0)
            fatal("select");
    }
    close(server_fd);
}
#pragma once

#include <sys/types.h>
#include <stdbool.h>

struct redirect
{
    bool is_out;
    const char *to;
    int to_fd;
    const char *from;
    int from_fd;
    struct redirect *next;
};

struct process
{
    pid_t pid, pgid;
    char **arguments;
    size_t argc;
    struct redirect *redirects;
    struct process *next;
};

struct job 
{
    pid_t pgid;
    bool fg;
    struct process *pipeline;
    struct job *next;
};

#include "utility.h"

static inline LINKED_LIST_ADD_FIRST(job)

struct job* remove_job(struct job **ptr, pid_t pid);
void empty_job(struct job *job);
void destroy_job(struct job *job);
void destroy_redirect(struct redirect * const redirect);

extern struct job *current_job;
extern int pending_jobs;
extern struct job *pending_removed_jobs;

void empty_process(struct process *process);
void destroy_process(struct process *process);

static inline LINKED_LIST_ADD_LAST(process)
static inline LINKED_LIST_ADD_LAST(redirect)

int start_job(struct job *job);

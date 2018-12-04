#pragma once

#include <sys/types.h>
#include <stdbool.h>

struct redirect
{
    bool is_out;
    const char *left;
    int left_fd;
    const char *right;
    int right_fd;
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
    bool stopped;
    bool falling_apart;
    struct process *pipeline;
    size_t pipeline_size;
    struct job *next;
};

extern struct job *current_job;
extern struct job *pending_jobs;
extern struct job *pending_removed_jobs;

#include "utility.h"

static inline LINKED_LIST_ADD_FIRST(job)
static inline LINKED_LIST_ADD_LAST(job)
static inline LINKED_LIST_ADD_LAST(process)
static inline LINKED_LIST_ADD_LAST(redirect)
static inline LINKED_LIST_SIZE(process)

struct job* remove_job(struct job **ptr, pid_t pid);
void empty_job(struct job *job);
void destroy_job(struct job *job);

void empty_process(struct process *process);
void destroy_process(struct process *process);

void destroy_redirect(struct redirect * const redirect);

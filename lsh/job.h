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
};

struct job_handle
{
    struct job *job;
    struct job_handle *next;
};

extern struct job_handle *current_job;
extern struct job_handle *pending_jobs;
extern struct job_handle *pending_removed_jobs;

#include "utility.h"

static inline LINKED_LIST_ADD_FIRST(job_handle)
static inline LINKED_LIST_ADD_LAST(job_handle)
static inline LINKED_LIST_ADD_LAST(process)
static inline LINKED_LIST_ADD_LAST(redirect)
static inline LINKED_LIST_SIZE(process)

struct job_handle* remove_job_handle(struct job_handle **ptr, pid_t pid);
void empty_job(struct job *job);
struct job_handle* make_job_handle(struct job *job);
void destroy_job(struct job *job);
void print_job(int index, const struct job *job, bool ended);

void empty_process(struct process *process);
void destroy_process(struct process *process);

void destroy_redirect(struct redirect * const redirect);

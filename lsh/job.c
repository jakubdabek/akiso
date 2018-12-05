#include "job.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct job_handle *current_job = NULL;
struct job_handle *pending_jobs = NULL;
struct job_handle *pending_removed_jobs = NULL;


struct job_handle* remove_job_handle(struct job_handle **ptr, pid_t pid)
{
    if (ptr == NULL)
        return NULL;

    while (*ptr != NULL)
    {
        if ((*ptr)->job->pgid == pid || pid <= 0)
        {
            struct job_handle *tmp = *ptr;
            *ptr = (*ptr)->next;
            tmp->next = NULL;
            return tmp;
        }
        ptr = &((*ptr)->next);
    }

    return NULL;
}

struct job_handle* make_job_handle(struct job *job)
{
    struct job_handle *handle = malloc(sizeof(*handle));
    handle->job = job;
    handle->next = NULL;

    return handle;
}

void empty_job(struct job *job)
{
    if (job == NULL)
        return;

    // job->pgid = 0;
    // job->fg = false;
    // job->stopped = false;
    // job->falling_apart = false;
    // job->pipeline = NULL;
    // job->pipeline_size = 0;
    // job->next = NULL;

    memset(job, 0, sizeof(*job));
}

void destroy_job(struct job *job)
{
    if (job == NULL)
        return;
    destroy_process(job->pipeline);
    free(job);
}

void print_job(int index, const struct job * const job, bool ended)
{
    if (index > 0)
        printf("[%d] ", index);
    if (job->stopped)
        printf("job: %8ld (Stopped)\n", (long)job->pgid);
    else if (ended)
        printf("job: %8ld (Done)\n", (long)job->pgid);
    else
        printf("job: %8ld (Running)\n", (long)job->pgid);
}

void empty_process(struct process *process)
{
    if (process == NULL)
        return;
    // process->pid = 0;
    // process->pgid = 0;
    // process->arguments = NULL;
    // process->argc = 0;
    // process->redirects = NULL;
    // process->next = NULL;

    memset(process, 0, sizeof(*process));
}

void destroy_redirect(struct redirect * const redirect)
{
    if (redirect == NULL)
        return;
    
    free((void*)redirect->left);
    free((void*)redirect->right);
    destroy_redirect(redirect->next);
    free(redirect);
}

void destroy_process(struct process * const process)
{
    if (process == NULL)
        return;
    
    free(process->arguments);
    destroy_redirect(process->redirects);
    destroy_process(process->next);
    free(process);
}

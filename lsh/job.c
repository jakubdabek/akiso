#include "job.h"

#include <stddef.h>
#include <stdlib.h>

struct job *current_job = NULL;
struct job *pending_jobs = NULL;
struct job *pending_removed_jobs = NULL;


struct job* remove_job(struct job **ptr, pid_t pid)
{
    if (ptr == NULL)
        return NULL;

    while (*ptr != NULL)
    {
        if ((*ptr)->pgid == pid || pid <= 0)
        {
            struct job *tmp = *ptr;
            *ptr = (*ptr)->next;
            return tmp;
        }
        ptr = &((*ptr)->next);
    }

    return NULL;
}

void empty_job(struct job *job)
{
    job->pgid = 0;
    job->fg = false;
    job->pipeline = NULL;
    job->next = NULL;
}

void destroy_job(struct job *job)
{
    if (job == NULL)
        return;
    destroy_process(job->pipeline);
    destroy_job(job->next);
    free(job);
}

void empty_process(struct process *process)
{
    process->pid = 0;
    process->pgid = 0;
    process->arguments = NULL;
    process->argc = 0;
    process->redirects = NULL;
    process->next = NULL;
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

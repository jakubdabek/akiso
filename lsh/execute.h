#include "job.h"
#include "parser.h"

int start_job(struct job *job);
//int wait_for_fg_job(struct job *job);

extern const char * const builtins[];
size_t builtins_count;

enum parse_result do_cd(const char * const *args, int argc);
enum parse_result do_jobs(const char * const *args, int argc);
enum parse_result do_fg(const char * const *args, int argc);
enum parse_result do_bg(const char * const *args, int argc);
enum parse_result do_exit(const char * const *args, int argc);

typedef enum parse_result (*builtin_command)(const char * const *args, int argc);

extern const builtin_command builtin_commands[];
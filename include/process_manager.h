#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>


void add_job(pid_t pid, const char *command);
void remove_job(pid_t pid);
void list_jobs();
void bring_job_to_foreground(int job_id);
void move_job_to_background(int job_id);

#endif // PROCESS_MANAGER_H

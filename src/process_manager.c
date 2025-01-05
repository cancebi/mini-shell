#include "../include/process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_JOBS 100


Job jobs[MAX_JOBS];
int job_count = 0;

void add_job(pid_t pid, const char *command) {
    if (job_count >= MAX_JOBS) {
        fprintf(stderr, "Job list is full.\n");
        return;
    }
    
    jobs[job_count].job_id = job_count + 1;
    jobs[job_count].pid = pid;
    strncpy(jobs[job_count].command, command, 255);
    jobs[job_count].command[255] = '\0';
    jobs[job_count].running = 1;
    job_count++;
    printf("[%d] %d\n", jobs[job_count - 1].job_id, pid);
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

void list_jobs() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s %s\n",
               jobs[i].job_id,
               jobs[i].pid,
               jobs[i].running ? "En cours d'exécution" : "Stoppé",
               jobs[i].command);
    }
}

void bring_job_to_foreground(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            printf("Bringing job [%d] %s to foreground.\n", job_id, jobs[i].command);
            kill(jobs[i].pid, SIGCONT);
            waitpid(jobs[i].pid, NULL, 0);
            remove_job(jobs[i].pid);
            return;
        }
    }
    fprintf(stderr, "Job [%d] not found.\n", job_id);
}

void move_job_to_background(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            if (jobs[i].running) {
                fprintf(stderr, "Job [%d] is already running in background.\n", job_id);
                return;
            }
            printf("Moving job [%d] %s to background.\n", job_id, jobs[i].command);
            jobs[i].running = 1;
            kill(jobs[i].pid, SIGCONT);
            return;
        }
    }
    fprintf(stderr, "Job [%d] not found.\n", job_id);
}

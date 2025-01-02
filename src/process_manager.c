#include "../include/process_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_JOBS 100

typedef struct {
    int job_id;
    pid_t pid;
    char command[256];
    int running;
} Job;

Job jobs[MAX_JOBS];
int job_count = 0;
int next_job_id = 1;


/**
 * @brief Ajoute un nouveau job à la liste des jobs en arrière-plan.
 * 
 * Cette fonction enregistre un nouveau job identifié par son PID et la commande associée.
 * Si la liste des jobs est pleine, un message d'erreur est affiché.
 * 
 * @param pid Le PID du processus du job.
 * @param command La commande associée au job.
 */
void add_job(pid_t pid, const char *command) {
    if (job_count >= MAX_JOBS) {
        fprintf(stderr, "Job list is full.\n");
        return;
    }

    jobs[job_count].job_id = next_job_id++;
    jobs[job_count].pid = pid;
    strncpy(jobs[job_count].command, command, 255);
    jobs[job_count].command[255] = '\0';
    jobs[job_count].running = 1;
    job_count++;
    printf("[%d] %d\n", jobs[job_count - 1].job_id, pid);
}


/**
 * @brief Supprime un job de la liste des jobs.
 * 
 * Cette fonction retire un job de la liste des jobs en fonction de son PID.
 * Les autres jobs sont décalés dans la liste pour combler l'espace.
 * 
 * @param pid Le PID du job à supprimer.
 */
void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            return;
        }
    }
}


/**
 * @brief Affiche tous les jobs actuellement en arrière-plan.
 * 
 * Cette fonction parcourt la liste des jobs et imprime les informations
 * sur chaque job, notamment son ID, son PID, son état ("Running" ou "Stopped")
 * et la commande associée.
 */
void list_jobs() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s %s\n", jobs[i].job_id, jobs[i].pid,
               jobs[i].running ? "Running" : "Stopped", jobs[i].command);
    }
}


/**
 * @brief Ramène un job en avant-plan.
 * 
 * Cette fonction reprend l'exécution d'un job spécifique et l'amène en avant-plan.
 * Le shell attend que le job se termine avant de reprendre.
 * 
 * @param job_id L'ID du job à ramener en avant-plan.
 */
void bring_job_to_foreground(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            int status;
            printf("Bringing job [%d] %s to foreground.\n", job_id, jobs[i].command);
            kill(jobs[i].pid, SIGCONT);
            jobs[i].running = 1;
            waitpid(jobs[i].pid, &status, 0);
            remove_job(jobs[i].pid);
            return;
        }
    }
    fprintf(stderr, "Job [%d] not found.\n", job_id);
}


/**
 * @brief Déplace un job en arrière-plan.
 * 
 * Cette fonction reprend l'exécution d'un job spécifique et le déplace
 * en arrière-plan pour permettre au shell de continuer son exécution.
 * 
 * @param job_id L'ID du job à déplacer en arrière-plan.
 */
void move_job_to_background(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            printf("Moving job [%d] %s to background.\n", job_id, jobs[i].command);
            kill(jobs[i].pid, SIGCONT);
            jobs[i].running = 1;
            return;
        }
    }
    fprintf(stderr, "Job [%d] not found.\n", job_id);
}

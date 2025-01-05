#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#include <sys/types.h>

/**
 * @brief Structure représentant un job en arrière-plan.
 */
typedef struct {
    int job_id;         ///< Identifiant unique du job.
    pid_t pid;          ///< PID du processus associé.
    char command[256];  ///< Commande exécutée.
    int running;        ///< 1 si en cours d'exécution, 0 si stoppé.
} Job;

extern Job jobs[];     ///< Tableau global des jobs.
extern int job_count;  ///< Nombre total de jobs.

/**
 * @brief Ajoute un nouveau job à la liste des jobs.
 * 
 * @param pid PID du processus.
 * @param command Commande associée au job.
 */
void add_job(pid_t pid, const char *command);

/**
 * @brief Supprime un job de la liste en fonction de son PID.
 * 
 * @param pid PID du processus à supprimer.
 */
void remove_job(pid_t pid);

/**
 * @brief Affiche la liste des jobs actifs.
 */
void list_jobs();

/**
 * @brief Ramène un job en avant-plan.
 * 
 * @param job_id Identifiant du job à ramener en avant-plan.
 */
void bring_job_to_foreground(int job_id);

/**
 * @brief Déplace un job en arrière-plan.
 * 
 * @param job_id Identifiant du job à déplacer en arrière-plan.
 */
void move_job_to_background(int job_id);

#endif // PROCESS_MANAGER_H

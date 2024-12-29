#include <stdio.h>
#include <stdlib.h>
#include <libproc.h>
#include <pwd.h>
#include <time.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <sys/proc_info.h>

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

// Fonction pour retourner la couleur en fonction de l'état du processus
const char* get_color(char state) {
    switch (state) {
        case 'R': return GREEN;  // Running
        case 'S': return YELLOW; // Sleeping
        case 'T': return RED;    // Stopped
        case 'Z': return RED;    // Zombie
        default: return RESET;   // Default
    }
}

// Fonction pour récupérer le statut du processus
char get_process_status(struct proc_bsdinfo *proc_info) {
    if (proc_info->pbi_flags & P_LTRACED) return 'T'; // Traced or stopped
    if (proc_info->pbi_flags & P_WEXIT) return 'Z';   // Zombie
    if (proc_info->pbi_flags & P_WAITED) return 'S';  // Sleeping
    if (proc_info->pbi_flags & P_SINTR) return 'I';   // Idle
    return 'R'; // Running (default)
}

// Fonction pour récupérer le nom d'utilisateur
const char* get_username(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : "unknown";
}

// Fonction pour formater le temps CPU
void format_time(uint64_t microseconds, char *buffer, size_t buffer_size) {
    uint64_t seconds = microseconds / 1000000;
    int hrs = seconds / 3600;
    int mins = (seconds % 3600) / 60;
    int secs = seconds % 60;
    snprintf(buffer, buffer_size, "%02d:%02d:%02d", hrs, mins, secs);
}

// Fonction principale pour afficher les processus
void myps() {
    int buffer_size = 1024;
    pid_t *pids = malloc(buffer_size * sizeof(pid_t));
    int num_pids = proc_listpids(PROC_ALL_PIDS, 0, pids, buffer_size * sizeof(pid_t)) / sizeof(pid_t);

    if (num_pids < 1) {
        perror("proc_listpids failed");
        free(pids);
        return;
    }

    printf("USER       PID  %%CPU %%MEM      VSZ    RSS   STAT STARTED      TIME     COMMAND\n");

    for (int i = 0; i < num_pids; i++) {
        if (pids[i] == 0) {
            continue;
        }

        struct proc_bsdinfo proc_info;
        struct proc_taskinfo task_info;
        char path_buffer[PROC_PIDPATHINFO_MAXSIZE] = "Unknown";

        if (proc_pidinfo(pids[i], PROC_PIDTBSDINFO, 0, &proc_info, sizeof(proc_info)) > 0 &&
            proc_pidinfo(pids[i], PROC_PIDTASKINFO, 0, &task_info, sizeof(task_info)) > 0) {

            // Filtrer les processus qui ne sont pas de l'utilisateur actuel
            if (proc_info.pbi_uid != getuid()) {
                continue;
            }

            // Récupérer le chemin complet du binaire
            if (proc_pidpath(pids[i], path_buffer, sizeof(path_buffer)) == 0) {
                snprintf(path_buffer, sizeof(path_buffer), "Unknown");
            }

            // Calculer l'état du processus
            char stat = get_process_status(&proc_info);

            // Récupérer l'heure de démarrage
            time_t start_time = proc_info.pbi_start_tvsec;
            struct tm *tm_info = localtime(&start_time);
            char time_buffer[16];
            strftime(time_buffer, sizeof(time_buffer), "%H:%M", tm_info);

            // Formater le temps CPU
            char cpu_time[16];
            format_time(task_info.pti_total_user, cpu_time, sizeof(cpu_time));

            printf("%-10s %5d  %.1f  %.1f %8llu %8llu  %c    %s  %s  %s\n",
                   get_username(proc_info.pbi_uid),                  // USER
                   pids[i],                                         // PID
                   task_info.pti_total_user / 1000000.0,            // %CPU (approximation)
                   task_info.pti_resident_size / 1024.0 / 1024.0,   // %MEM (approximation)
                   task_info.pti_virtual_size / 1024,               // VSZ
                   task_info.pti_resident_size / 1024,              // RSS
                   stat,                                            // STAT
                   time_buffer,                                     // STARTED
                   cpu_time,                                        // TIME
                   path_buffer                                      // COMMAND
            );
        }
    }

    free(pids);
}

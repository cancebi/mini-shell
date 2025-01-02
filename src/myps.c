#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/sysinfo.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#define MAX_PATH 512
#define MAX_CMD 256

/**
 * @brief Récupère le nom d'utilisateur correspondant à un UID donné.
 * 
 * @param uid L'UID de l'utilisateur.
 * @return const char* Le nom d'utilisateur ou "unknown" si non trouvé.
 */
const char *get_username(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : "unknown";
}

/**
 * @brief Calcule l'utilisation du CPU d'un processus en pourcentage.
 * 
 * @param utime Temps utilisateur consommé par le processus.
 * @param stime Temps système consommé par le processus.
 * @param start_time Temps de démarrage du processus.
 * @param uptime Temps d'activité total du système.
 * @param clk_tck Ticks par seconde (valeur de sysconf(_SC_CLK_TCK)).
 * @return float Pourcentage d'utilisation du CPU.
 */
float calculate_cpu(unsigned long utime, unsigned long stime, unsigned long start_time, long uptime, long clk_tck) {
    unsigned long total_time = utime + stime;
    float seconds = uptime - (start_time / clk_tck);
    return (seconds > 0) ? 100.0 * ((float)total_time / clk_tck) / seconds : 0.0;
}

/**
 * @brief Calcule l'utilisation de la mémoire d'un processus en pourcentage.
 * 
 * @param rss Résident Set Size (taille mémoire occupée en RAM).
 * @param totalram Taille totale de la RAM du système.
 * @return float Pourcentage d'utilisation de la mémoire.
 */
float calculate_mem(unsigned long rss, unsigned long totalram) {
    return (totalram > 0) ? 100.0 * ((float)rss / totalram) : 0.0;
}

/**
 * @brief Récupère le terminal (TTY) associé à un processus.
 * 
 * @param proc_path Chemin du répertoire du processus dans `/proc`.
 * @return const char* Le nom du terminal associé ou "?" si aucun.
 */
const char *get_tty(const char *proc_path) {
    static char tty[16] = "?";
    char fd_path[MAX_PATH];
    snprintf(fd_path, sizeof(fd_path), "%s/fd/0", proc_path);

    char link_path[MAX_PATH];
    ssize_t len = readlink(fd_path, link_path, sizeof(link_path) - 1);
    if (len > 0) {
        link_path[len] = '\0';
        if (strstr(link_path, "/dev/pts")) {
            snprintf(tty, sizeof(tty), "%s", strrchr(link_path, '/') + 1);
        } else if (strstr(link_path, "/dev/tty")) {
            snprintf(tty, sizeof(tty), "%s", strrchr(link_path, '/') + 1);
        }
    }
    return tty;
}

/**
 * @brief Convertit le temps de démarrage d'un processus en une chaîne lisible.
 * 
 * @param start_time Temps de démarrage du processus en ticks.
 * @param clk_tck Ticks par seconde.
 * @param boot_time Temps de démarrage du système.
 * @param start_buffer Buffer pour stocker la chaîne de temps.
 * @param buffer_size Taille du buffer.
 */
void get_start_time(unsigned long start_time, long clk_tck, time_t boot_time, char *start_buffer, size_t buffer_size) {
    time_t process_start_time = boot_time + (start_time / clk_tck);
    struct tm *local_time = localtime(&process_start_time);
    if (local_time) {
        strftime(start_buffer, buffer_size, "%H:%M", local_time);
    } else {
        strncpy(start_buffer, "unknown", buffer_size);
    }
}

/**
 * @brief Convertit le temps CPU consommé par un processus en une chaîne lisible.
 * 
 * @param utime Temps utilisateur consommé.
 * @param stime Temps système consommé.
 * @param clk_tck Ticks par seconde.
 * @param time_buffer Buffer pour stocker la chaîne de temps.
 * @param buffer_size Taille du buffer.
 */
void get_cpu_time(unsigned long utime, unsigned long stime, long clk_tck, char *time_buffer, size_t buffer_size) {
    unsigned long total_time = (utime + stime) / clk_tck;
    unsigned long minutes = total_time / 60;
    unsigned long seconds = total_time % 60;
    snprintf(time_buffer, buffer_size, "%02lu:%02lu", minutes, seconds);
}


/**
 * @brief Récupère et affiche les informations d'un processus.
 * 
 * Affiche des détails tels que le nom d'utilisateur, l'utilisation CPU, l'utilisation mémoire,
 * l'état du processus, le terminal, le temps de démarrage, et la commande exécutée.
 * 
 * @param pid L'identifiant (PID) du processus.
 * @param proc_path Chemin du répertoire du processus dans `/proc`.
 * @param sys_info Informations système (structure `sysinfo`).
 */
void get_process_info(const char *pid, const char *proc_path, struct sysinfo *sys_info) {
    char stat_path[MAX_PATH], status_path[MAX_PATH], cmdline_path[MAX_PATH], comm_path[MAX_PATH];
    snprintf(stat_path, MAX_PATH, "%s/stat", proc_path);
    snprintf(status_path, MAX_PATH, "%s/status", proc_path);
    snprintf(cmdline_path, MAX_PATH, "%s/cmdline", proc_path);
    snprintf(comm_path, MAX_PATH, "%s/comm", proc_path);

    FILE *stat_file = fopen(stat_path, "r");
    FILE *status_file = fopen(status_path, "r");
    FILE *cmdline_file = fopen(cmdline_path, "r");

    if (!stat_file || !status_file) {
        if (stat_file) fclose(stat_file);
        if (status_file) fclose(status_file);
        return;
    }

    char comm[256], state;
    unsigned long utime, stime, vsize, rss, start_time;
    uid_t uid = -1;
    char cmdline[MAX_CMD] = "[unknown]";
    char start_buffer[16] = "unknown";
    char time_buffer[16] = "00:00";   

    fscanf(stat_file, "%*d %s %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %lu %lu %lu",
           comm, &state, &utime, &stime, &vsize, &rss, &start_time);
    fclose(stat_file);

    char line[256];
    while (fgets(line, sizeof(line), status_file)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid: %u", &uid);
            break;
        }
    }
    fclose(status_file);

    if (cmdline_file) {
        size_t len = fread(cmdline, 1, sizeof(cmdline) - 1, cmdline_file);
        if (len > 0) {
            cmdline[len] = '\0';
        } else {
            snprintf(cmdline, sizeof(cmdline), "[%.250s]", comm);
        }
        fclose(cmdline_file);
    } else {
        snprintf(cmdline, sizeof(cmdline), "[%.250s]", comm);
    }


    time_t boot_time = time(NULL) - sys_info->uptime;
    get_start_time(start_time, sysconf(_SC_CLK_TCK), boot_time, start_buffer, sizeof(start_buffer));
    get_cpu_time(utime, stime, sysconf(_SC_CLK_TCK), time_buffer, sizeof(time_buffer));

    printf("%-10s %5s %5.1f %5.1f %8lu %7lu %-6s %c %-5s %-5s %s\n",
           get_username(uid), pid,
           calculate_cpu(utime, stime, start_time, sys_info->uptime, sysconf(_SC_CLK_TCK)),
           calculate_mem(rss * sysconf(_SC_PAGESIZE), sys_info->totalram),
           vsize / 1024, rss * sysconf(_SC_PAGESIZE) / 1024,
           get_tty(proc_path), state, start_buffer, time_buffer, cmdline);
}


/**
 * @brief Affiche la liste des processus en cours d'exécution sur le système.
 * 
 * Parcourt le répertoire `/proc`, récupère les informations de chaque processus et
 * les affiche sous forme tabulaire avec des détails tels que l'utilisateur, le PID,
 * l'utilisation CPU, l'utilisation mémoire, et la commande associée.
 */
void myps() {
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    struct sysinfo sys_info;
    sysinfo(&sys_info);

    printf("USER       PID  %%CPU %%MEM      VSZ    RSS   TTY   STAT START TIME  COMMAND\n");

    while ((entry = readdir(proc_dir)) != NULL) {
        if (!isdigit(entry->d_name[0])) {
            continue;
        }

        char proc_path[MAX_PATH];
        int len = snprintf(proc_path, MAX_PATH, "/proc/%s", entry->d_name);
        if (len >= MAX_PATH) {
            fprintf(stderr, "Path truncated: %s\n", entry->d_name);
            continue;
        }

        get_process_info(entry->d_name, proc_path, &sys_info);
    }

    closedir(proc_dir);
}

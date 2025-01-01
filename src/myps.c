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

// Fonction pour obtenir le nom d'utilisateur à partir de l'UID
const char *get_username(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : "unknown";
}

// Fonction pour calculer l'utilisation CPU (approximation)
float calculate_cpu(unsigned long utime, unsigned long stime, unsigned long start_time, long uptime, long clk_tck) {
    unsigned long total_time = utime + stime;
    float seconds = uptime - (start_time / clk_tck);
    return (seconds > 0) ? 100.0 * ((float)total_time / clk_tck) / seconds : 0.0;
}

// Fonction pour calculer l'utilisation mémoire (en pourcentage)
float calculate_mem(unsigned long rss, unsigned long totalram) {
    return (totalram > 0) ? 100.0 * ((float)rss / totalram) : 0.0;
}

// Fonction pour obtenir le terminal associé au processus
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

// Fonction pour convertir le temps de démarrage en format lisible
void get_start_time(unsigned long start_time, long clk_tck, time_t boot_time, char *start_buffer, size_t buffer_size) {
    time_t process_start_time = boot_time + (start_time / clk_tck);
    struct tm *local_time = localtime(&process_start_time);
    if (local_time) {
        strftime(start_buffer, buffer_size, "%H:%M", local_time);
    } else {
        strncpy(start_buffer, "unknown", buffer_size);
    }
}

// Fonction pour convertir le temps CPU utilisé en format lisible
void get_cpu_time(unsigned long utime, unsigned long stime, long clk_tck, char *time_buffer, size_t buffer_size) {
    unsigned long total_time = (utime + stime) / clk_tck;
    unsigned long minutes = total_time / 60;
    unsigned long seconds = total_time % 60;
    snprintf(time_buffer, buffer_size, "%02lu:%02lu", minutes, seconds);
}

// Fonction pour récupérer les informations d'un processus
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
    char start_buffer[16] = "unknown";  // Pour START
    char time_buffer[16] = "00:00";     // Pour TIME

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
            // Toujours ajouter des crochets, même si comm contient des parenthèses
            snprintf(cmdline, sizeof(cmdline), "[%.250s]", comm);
        }
        fclose(cmdline_file);
    } else {
        // Toujours ajouter des crochets, même si comm contient des parenthèses
        snprintf(cmdline, sizeof(cmdline), "[%.250s]", comm);
    }





    // Calcul START et TIME
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

// Fonction principale pour lister les processus
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

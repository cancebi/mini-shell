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

// Fonction pour récupérer les informations d'un processus
void get_process_info(const char *pid, const char *proc_path, struct sysinfo *sys_info) {
    char stat_path[MAX_PATH], status_path[MAX_PATH], cmdline_path[MAX_PATH];
    snprintf(stat_path, MAX_PATH, "%s/stat", proc_path);
    snprintf(status_path, MAX_PATH, "%s/status", proc_path);
    snprintf(cmdline_path, MAX_PATH, "%s/cmdline", proc_path);

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
        if (len == 0) {
            strncpy(cmdline, "[kernel thread]", sizeof(cmdline) - 1);
        }
        cmdline[len] = '\0';
        fclose(cmdline_file);
    } else {
        strncpy(cmdline, "[unreadable]", sizeof(cmdline) - 1);
    }

    unsigned long vsize_kb = vsize / 1024;
    unsigned long rss_kb = rss * sysconf(_SC_PAGESIZE) / 1024;

    char tty[16];
    snprintf(tty, sizeof(tty), "%s", get_tty(proc_path));
    float cpu_usage = calculate_cpu(utime, stime, start_time, sys_info->uptime, sysconf(_SC_CLK_TCK));
    float mem_usage = calculate_mem(rss * sysconf(_SC_PAGESIZE), sys_info->totalram);

    printf("%-10s %5s %5.1f %5.1f %8lu %7lu %-6s %c %s\n",
           get_username(uid), pid, cpu_usage, mem_usage, vsize_kb, rss_kb, tty, state, cmdline);
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

    printf("USER       PID  %%CPU %%MEM      VSZ    RSS   TTY   STAT COMMAND\n");

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

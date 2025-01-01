//executor.h
#ifndef EXECUTOR_H
#define EXECUTOR_H


// Executes a command and returns the exit status
int execute_command(char *command);

#endif // EXECUTOR_H


//executor.c
#include "executor.h"
#include "wildcard.h"
#include "myls.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "myps.h"

int execute_command(char *command) {
    pid_t pid = fork();
    int status;

    if (pid == 0) {
        // Child process: execute the command with arguments
        char *args[100];
        int i = 0;

        // Split the command into tokens
        char *token = strtok(command, " "); // Splitting by space
        while (token != NULL && i < 99) {
            int has_wildcard = (strchr(token, '*') != NULL || strchr(token, '?') != NULL || strchr(token, '[') != NULL);
            int is_escaped_token = 0;

            // Check if the token has any backslash
            for (int j = 0; token[j] != '\0'; j++) {
                if (is_escaped(token, j)) {
                    is_escaped_token = 1;
                    break;
                }
            }

            // If the token has a wildcard and is not escaped, expand it
            if (has_wildcard && !is_escaped_token) {
                int num_matches = 0;
                char **matches = expand_wildcard(token, &num_matches); // Use expand_wildcard for *, ?, [
                if (matches) {
                    for (int j = 0; j < num_matches && i < 99; j++) {
                        args[i++] = matches[j];
                    }
                    free(matches);
                }
            } else {
                args[i++] = token;
            }
            token = strtok(NULL, " ");
        }
        args[i] = NULL;  // Null-terminate the args array

        if (strcmp(args[0], "myls") == 0) {
            myls_run(i, args);
            exit(0);
        } else if (strcmp(args[0], "myps") == 0) {
            myps(); // Appeler la fonction myps
            exit(0);
        } else{
            if (execvp(args[0], args) == -1) {
                perror("execvp failed");
                exit(127);  // Non-zero exit status for failure
            }
        }
    } else if (pid > 0) {
        // Parent process: wait for the child to complete
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            return 127;  // Indicate failure to execute
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);  // Return the child's exit status
        }
        return 127;  // Non-zero if child didn't terminate normally
    } else {
        perror("fork failed");
    }
    return 127;  // Return non-zero if fork fails
}



//mysh.h
#ifndef MYSH_H
#define MYSH_H

// Function to start the shell loop
void run_shell();
void handle_sigint(int sig);
int change_directory(char *path);
void print_status();
#endif // MYSH_H


//mysh.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "executor.h"
#include "parser.h"


#define ROUGE(x) "\033[31m" x "\033[0m"
#define VERT(x) "\033[32m" x "\033[0m"

#define MAX_COMMAND_LENGTH 100
#define MAX_PATH_LENGTH 1024

volatile sig_atomic_t foreground_running = 0;
char last_command_name[MAX_COMMAND_LENGTH] = "";
int last_status = -1;
char current_directory[MAX_PATH_LENGTH];  
char global_command_line[MAX_COMMAND_LENGTH];

void handle_sigint(int sig){
    if (foreground_running){
        return;
    }
    printf("\nDo you want to quit mysh? (y/n): ");
    fflush(stdout);

    int response = getchar();
    while (getchar() != '\n'); // Clear any additional input characters

    if (response == 'y' || response == 'Y') {
        printf("Exiting mysh.\n");
        exit(0);
    }
    else{
        printf("Continuing mysh. \n");
        if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
            perror("getcwd failed");
            strcpy(current_directory, "?");
        }
        printf("mysh:%s> ", current_directory);
        fflush(stdout);
    }
}

int change_directory(char *path) {
    if (path == NULL || strcmp(path, "~") == 0) {
        path = getenv("HOME");  // Default to home directory
    }

    if (chdir(path) != 0) {
        perror("cd failed");
        return 1;  // Non-zero status for failure
    }
    return 0;  // Zero status for success
}

void print_status() {
    if (last_status == -1) {
        printf("No commands have been executed yet.\n");
    } else {
        if (last_status == 0) {
            printf(VERT("%s terminé avec comme code de retour %d\n"), last_command_name, last_status);
        } else {
            printf(ROUGE("%s terminé anormalement\n"), last_command_name);
        }
    } 
    strcpy(last_command_name, "status");
    last_status = 0;
}

void run_shell() {
    int num_commands;

    signal(SIGINT, handle_sigint);

    while (1) {
        if(getcwd(current_directory, sizeof(current_directory)) == NULL) {
            perror("getcwd failed");
            strcpy(current_directory, "?"); 
        }
        printf("mysh:%s ~> ", current_directory);
        fflush(stdout);

        ssize_t n = read(STDIN_FILENO, global_command_line, MAX_COMMAND_LENGTH - 1);
        if (n > 0) {
            global_command_line[n] = '\0'; // Null-terminate the input
            size_t length = strcspn(global_command_line, "\n");
            global_command_line[length] = '\0'; // Remove newline if present
        } else if (n == -1) {
            perror("read error");
            continue;
        }

        if (strlen(global_command_line) == 0) {
            continue; // Skip to the next iteration if input is empty
        }

        if (strspn(global_command_line, " ") == strlen(global_command_line)) {
            continue; // Skip to the next iteration if input is only whitespace
        }

        if(strncmp(global_command_line, "cd", 2) == 0 ){
            char *directory = strtok(global_command_line + 3, " ");
            last_status = change_directory(directory);
            strncpy(last_command_name, "cd", MAX_COMMAND_LENGTH);
            continue;
        }

        if (strcmp(global_command_line, "exit") == 0) {
            printf("Exiting mysh.\n");
            exit(0);
        }

        if (strcmp(global_command_line, "status") == 0) {
            print_status();
            continue;
        }

        ParsedCommand *commands = parse_input(global_command_line, &num_commands);

        for (int i = 0; i < num_commands; i++) {
            bool should_run = (i == 0) ||
                              (commands[i - 1].condition == COND_SUCCESS && last_status == 0) ||
                              (commands[i - 1].condition == COND_FAILURE && last_status != 0) ||
                              (commands[i - 1].condition == COND_ALWAYS);

            if (should_run) {
                foreground_running = 1;
                strncpy(last_command_name, commands[i].command, MAX_COMMAND_LENGTH);
                last_status = execute_command(commands[i].command);
                foreground_running = 0;
            }
        }

        for (int i = 0; i < num_commands; i++) {
            free(commands[i].command);
        }
        free(commands);
    }
}

int main() {
    run_shell();
    return 0;
}


//myls.h
#ifndef MYLS_H
#define MYLS_H

void myls_run(int argc, char *argv[]);

#endif


//myls.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/xattr.h>

// Fonction pour gérer les attributs étendus
char get_extended_attributes(const char *path) {
    #ifdef __APPLE__
        ssize_t attr_count = listxattr(path, NULL, 0, XATTR_NOFOLLOW);
    #else
        ssize_t attr_count = listxattr(path, NULL, 0);
    #endif
        return (attr_count > 0) ? '@' : ' ';
}
void print_colored(const char *name, mode_t mode) {
    if (S_ISDIR(mode)) {
        printf("\033[34m%s\033[0m", name); // Blue for directories
    } else if (S_ISLNK(mode)) {
        printf("\033[36m%s\033[0m", name); // Cyan for symlinks
    } else if (mode & S_IXUSR) {
        printf("\033[32m%s\033[0m", name); // Green for executables
    } else {
        printf("%s", name); // Default color for regular files
    }
}



void list_directory(const char *dir_path, bool show_all, bool recursive) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    struct stat stat_buf;
    char *files[1024];
    int file_count = 0;

    // Collecter les fichiers
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue; // Ignorer les fichiers cachés sauf si -a est spécifié
        }
        files[file_count] = strdup(entry->d_name);
        file_count++;
    }

    // Trier les fichiers
    qsort(files, file_count, sizeof(char *), (int (*)(const void *, const void *))strcmp);

    // Calcul du total
    int total_blocks = 0;
    for (int i = 0; i < file_count; i++) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, files[i]);

        if (lstat(full_path, &stat_buf) == -1) {
            continue;
        }

        total_blocks += stat_buf.st_blocks;
    }

    printf("\n%s:\n", dir_path);
    printf("total %d\n", total_blocks);

    // Afficher les fichiers
    for (int i = 0; i < file_count; i++) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, files[i]);

        if (lstat(full_path, &stat_buf) == -1) {
            continue;
        }

        char date[20];
        struct tm *timeinfo = localtime(&stat_buf.st_mtime);
        strftime(date, sizeof(date), "%b %d %H:%M", timeinfo);

        char ext_attr = get_extended_attributes(full_path);
        printf("%c%c%c%c%c%c%c%c%c%c%c %3lu %-8s %-8s %8lld %s ",
               S_ISDIR(stat_buf.st_mode) ? 'd' : (S_ISLNK(stat_buf.st_mode) ? 'l' : '-'),
               stat_buf.st_mode & S_IRUSR ? 'r' : '-',
               stat_buf.st_mode & S_IWUSR ? 'w' : '-',
               stat_buf.st_mode & S_IXUSR ? 'x' : '-',
               stat_buf.st_mode & S_IRGRP ? 'r' : '-',
               stat_buf.st_mode & S_IWGRP ? 'w' : '-',
               stat_buf.st_mode & S_IXGRP ? 'x' : '-',
               stat_buf.st_mode & S_IROTH ? 'r' : '-',
               stat_buf.st_mode & S_IWOTH ? 'w' : '-',
               stat_buf.st_mode & S_IXOTH ? 'x' : '-',
               ext_attr,
               stat_buf.st_nlink,
               getpwuid(stat_buf.st_uid)->pw_name,
               getgrgid(stat_buf.st_gid)->gr_name,
               (long long)stat_buf.st_size,
               date);

        print_colored(files[i], stat_buf.st_mode);

        if (S_ISLNK(stat_buf.st_mode)) {
            char link_target[1024];
            ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                printf(" -> %s", link_target);
            }
        }

        printf("\n");
    }

    // Gestion récursive des sous-répertoires
    if (recursive) {
        for (int i = 0; i < file_count; i++) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, files[i]);

            if (lstat(full_path, &stat_buf) == -1) {
                continue;
            }

            if (S_ISDIR(stat_buf.st_mode)) {
                // Ignorer "." et ".." dans la récursion
                if (strcmp(files[i], ".") == 0 || strcmp(files[i], "..") == 0) {
                    continue;
                }

                list_directory(full_path, show_all, recursive);
            }
        }
    }

    // Libérer les ressources
    for (int i = 0; i < file_count; i++) {
        free(files[i]);
    }

    closedir(dir); // Toujours fermer le descripteur de répertoire
}



void parse_options(int argc, char *argv[], bool *show_all, bool *recursive, int *dir_index) {
    *show_all = false;
    *recursive = false;
    *dir_index = argc; // Default to indicate no directories given

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'a':
                        *show_all = true;
                        break;
                    case 'R':
                        *recursive = true;
                        break;
                    default:
                        fprintf(stderr, "Unknown option: -%c\n", argv[i][j]);
                        exit(EXIT_FAILURE);
                }
            }
        } else {
            *dir_index = i;
            break;
        }
    }
}

int myls_run(int argc, char *argv[]) {
    bool show_all, recursive;
    int dir_index;
    parse_options(argc, argv, &show_all, &recursive, &dir_index);

    if (dir_index == argc) {
        list_directory(".", show_all, recursive); // Default to current directory
    } else {
        for (int i = dir_index; i < argc; i++) {
            char *dir = argv[i];

            if (dir[0] == '~') {
                char *home = getenv("HOME");
                if (home) {
                    char full_path[1024];
                    snprintf(full_path, sizeof(full_path), "%s%s", home, dir + 1);
                    dir = full_path;
                }
            }

            printf("\nListing directory: %s\n", dir);
            list_directory(dir, show_all, recursive);
        }
    }
    return 0;
}


//myps.h
#ifndef MYPS_H
#define MYPS_H

const char* get_color(int state) ;
void myps();


#endif // MYPS_H

//myps.c
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


//wildcard.h
#ifndef WILDCARD_H
#define WILDCARD_H

#include <stdbool.h>

char **expand_wildcard(const char *pattern, int *num_matches);
bool is_escaped(const char *pattern, int index);

#endif


//wildcard.c
#include "wildcard.h"
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#include <stdbool.h>

bool is_escaped(const char *pattern, int index) {
    return (index > 0 && pattern[index - 1] == '\\');
}

char **expand_wildcard(const char *pattern, int *num_matches) {
    // Extract directory path and pattern
    const char *last_slash = strrchr(pattern, '/');
    char dir_path[1024] = ".";
    const char *search_pattern = pattern;

    if (last_slash) {
        size_t dir_len = last_slash - pattern;
        if (dir_len >= sizeof(dir_path)) {
            fprintf(stderr, "Directory path is too long.\n");
            return NULL;
        }
        strncpy(dir_path, pattern, dir_len);
        dir_path[dir_len] = '\0';
        search_pattern = last_slash + 1;
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("opendir failed");
        return NULL;
    }

    char **matches = malloc(100 * sizeof(char *));
    if (!matches) {
        perror("malloc failed");
        closedir(dir);
        return NULL;
    }

    *num_matches = 0;
    size_t capacity = 100;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files unless explicitly matched
        if (entry->d_name[0] == '.' && search_pattern[0] != '.') {
            continue;
        }

        if (fnmatch(search_pattern, entry->d_name, FNM_NOESCAPE) == 0) {
            size_t full_path_len = strlen(dir_path) + 1 + strlen(entry->d_name) + 1; // +1 for '/' and +1 for '\0'
            if (full_path_len > 1024) {
                fprintf(stderr, "Path is too long: %s/%s\n", dir_path, entry->d_name);
                continue;
            }

            char *full_match = malloc(full_path_len);
            if (!full_match) {
                perror("malloc failed");
                continue;
            }

            snprintf(full_match, full_path_len, "%s/%s", dir_path, entry->d_name);
            matches[*num_matches] = full_match;
            (*num_matches)++;

            // Resize matches array if necessary
            if (*num_matches >= capacity) {
                capacity += 100;
                char **temp = realloc(matches, capacity * sizeof(char *));
                if (!temp) {
                    perror("realloc failed");
                    break;
                }
                matches = temp;
            }
        }
    }

    closedir(dir);
    return matches;
}

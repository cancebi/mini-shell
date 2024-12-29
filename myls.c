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

char get_extended_attributes(const char *path) {
    ssize_t attr_count = listxattr(path, NULL, 0, XATTR_NOFOLLOW);
    if (attr_count > 0) return '@';
    return ' ';
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

void list_directoryy(const char *dir_path, bool show_all, bool recursive) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    struct stat stat_buf;

    // Calculer le total
    int total_blocks = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (lstat(full_path, &stat_buf) == -1) {
            perror("lstat");
            continue;
        }

        total_blocks += stat_buf.st_blocks; // Accumuler les blocs
    }

    // Réinitialiser la lecture des fichiers dans le répertoire
    rewinddir(dir);

    // Afficher le total
    printf("total %d\n", total_blocks); // Alignement avec `ls -l` (blocs de 512 octets)

    // Lister les fichiers
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (lstat(full_path, &stat_buf) == -1) {
            perror("lstat");
            continue;
        }

        char date[20];
        struct tm *timeinfo = localtime(&stat_buf.st_mtime);
        strftime(date, sizeof(date), "%b %d %H:%M", timeinfo);

        printf("%c%c%c%c%c%c%c%c%c%c %3hu %-8s %-8s %8lld %s ",
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
               stat_buf.st_nlink,
               getpwuid(stat_buf.st_uid)->pw_name,
               getgrgid(stat_buf.st_gid)->gr_name,
               (long long)stat_buf.st_size,
               date);

        print_colored(entry->d_name, stat_buf.st_mode);

        if (S_ISLNK(stat_buf.st_mode)) {
            char link_target[1024];
            ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);
            if (len != -1) {
                link_target[len] = '\0';
                printf(" -> %s", link_target);
            }
        }

        printf("\n");

        if (recursive && S_ISDIR(stat_buf.st_mode) &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {
            printf("\n%s:\n", full_path);
            list_directoryy(full_path, show_all, recursive);
        }
    }

    closedir(dir);
}



void list_directoryyy(const char *dir_path, bool show_all, bool recursive) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    struct stat stat_buf;
    char *files[1024];
    int file_count = 0;

    // Collecter les fichiers pour les trier
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
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
    printf("total %d\n", total_blocks / 2);

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
        printf("%c%c%c%c%c%c%c%c%c%c%c %3hu %-8s %-8s %8lld %s ",
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
                list_directoryyy(full_path, show_all, recursive);
            }
        }
    }

    // Libérer les ressources
    for (int i = 0; i < file_count; i++) {
        free(files[i]);
    }

    closedir(dir);
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
        printf("%c%c%c%c%c%c%c%c%c%c%c %3hu %-8s %-8s %8lld %s ",
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

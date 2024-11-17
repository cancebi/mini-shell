#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>


void list_directory(const char *dir_path, bool show_all, bool recursive) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip hidden files if -a is not set
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }

        // Print entry
        printf("%s\n", entry->d_name);

        // If recursive, check if the entry is a directory using stat()
        if (recursive && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

            struct stat stat_buf;
            if (stat(path, &stat_buf) == 0 && S_ISDIR(stat_buf.st_mode)) {
                printf("\n%s:\n", path); // Print subdirectory header
                list_directory(path, show_all, recursive); // Recursively list the subdirectory
            }
        }
    }

    closedir(dir);
}

void parse_options(int argc, char *argv[], bool *show_all, bool *recursive, int *dir_index) {
    *show_all = false;
    *recursive = false;
    *dir_index = argc; // Default to indicate no directories given

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // Parse each character in the option
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

            // Handle ~ expansion for home directory
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

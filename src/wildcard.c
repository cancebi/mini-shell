#include "../include/wildcard.h"
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

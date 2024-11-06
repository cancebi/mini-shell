#include "wildcard.h"
#include <stdio.h> 
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>

char **expand_wildcard(const char *pattern, int *num_matches) {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir failed");
        return NULL;
    }

    char **matches = malloc(100 * sizeof(char *));
    *num_matches = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (fnmatch(pattern, entry->d_name, FNM_PERIOD) == 0) {
            matches[*num_matches] = strdup(entry->d_name);
            (*num_matches)++;
        }
    }
    closedir(dir);
    return matches;
}

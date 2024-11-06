#include "wildcard.h"
#include <stdio.h> 
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#include <stdbool.h>

bool is_escaped(const char *pattern, int index){
    return (index > 0 && pattern[index - 1] == '\\');
}

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
        bool match = true;
        for (int i = 0; pattern[i] != '\0'; i++) {
            if ((pattern[i] == '*' || pattern[i] == '?' || pattern[i] == '[') && is_escaped(pattern, i)) {
                if (strncmp(entry->d_name, pattern, strlen(pattern)) != 0) {
                    match = false;
                    break;
                }
            } else {
                match = (fnmatch(pattern, entry->d_name, 0) == 0);
            }
        }
        
        if (match) {
            matches[*num_matches] = strdup(entry->d_name);
            (*num_matches)++;
        }
    }
    closedir(dir);
    return matches;
}

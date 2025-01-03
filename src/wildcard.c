#include "../include/wildcard.h"
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <fnmatch.h>
#include <stdbool.h>


/**
 * @brief Vérifie si un caractère à une position donnée dans une chaîne est échappé.
 * 
 * Cette fonction détermine si un caractère dans une chaîne est précédé d'un caractère 
 * d'échappement (`\`) et ne doit donc pas être interprété comme un caractère spécial.
 * 
 * @param pattern La chaîne de caractères à vérifier.
 * @param index L'index du caractère à analyser.
 * @return bool Retourne `true` si le caractère est échappé, sinon `false`.
 */
bool is_escaped(const char *pattern, int index) {
    return (index > 0 && pattern[index - 1] == '\\');
}


/**
 * @brief Étend un motif contenant des jokers (`*`, `?`, etc.) en une liste de correspondances.
 * 
 * Cette fonction analyse un motif contenant des caractères spéciaux (jokers) et recherche 
 * tous les fichiers ou répertoires correspondants dans le chemin spécifié. Les résultats
 * sont retournés sous forme de tableau de chaînes.
 * 
 * @param pattern Le motif contenant des jokers à étendre.
 * @param num_matches Un pointeur vers un entier pour stocker le nombre de correspondances trouvées.
 * @return char** Un tableau de chaînes contenant les chemins des fichiers correspondants.
 *         Retourne `NULL` si une erreur survient ou si aucune correspondance n'est trouvée.
 * 
 * @note L'utilisateur est responsable de libérer la mémoire des chaînes dans le tableau retourné.
 */
char **expand_wildcard(const char *pattern, int *num_matches) {
    
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
        
        if (entry->d_name[0] == '.' && search_pattern[0] != '.') {
            continue;
        }

        if (fnmatch(search_pattern, entry->d_name, FNM_NOESCAPE) == 0) {
            size_t full_path_len = strlen(dir_path) + 1 + strlen(entry->d_name) + 1;
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
void free_matches(char **matches, int num_matches) {
    for (int i = 0; i < num_matches; i++) {
        free(matches[i]);
    }
    free(matches);
}

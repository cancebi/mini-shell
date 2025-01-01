#include "executor.h"
#include "redirection.h"
#include "wildcard.h"
#include "myls.h"
#include "myps.h"
#include "parser.h"
#include "process_manager.h"
#include "variable.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int background = 0;

int execute_command(char *command) {

    substitute_variables(command); // Substituer les variables

    // Vérifier la présence d'un pipeline
    if (strchr(command, '|')) {
        return handle_pipeline(command);
    }

    // Vérifier si la commande doit être exécutée en arrière-plan
    char *ampersand = strrchr(command, '&');
    if (ampersand && *(ampersand + 1) == '\0') {
        background = 1;
        *ampersand = '\0'; // Supprimer le '&' de la commande
        while (*(ampersand - 1) == ' ') {
            *(--ampersand) = '\0'; // Supprimer les espaces précédents
        }
    }

    pid_t pid = fork();
    int status;

    if (pid == 0) {
        // Processus enfant : gérer les redirections et exécuter la commande
        handle_redirection(command);

        char *args[100];
        int i = 0;

        // Découper la commande en arguments
        char *token = strtok(command, " ");
        while (token != NULL && i < 99) {
            int has_wildcard = (strchr(token, '*') != NULL || strchr(token, '?') != NULL || strchr(token, '[') != NULL);
            int is_escaped_token = 0;

            remove_quotes(token); // Supprimer les guillemets

            // Vérifier si le token contient un caractère échappé
            for (int j = 0; token[j] != '\0'; j++) {
                if (is_escaped(token, j)) {
                    is_escaped_token = 1;
                    break;
                }
            }

            // Si le token contient un joker non échappé, l'étendre
            if (has_wildcard && !is_escaped_token) {
                int num_matches = 0;
                char **matches = expand_wildcard(token, &num_matches);
                if (matches) {
                    for (int j = 0; j < num_matches && i < 99; j++) {
                        args[i++] = matches[j];
                    }
                    for (int j = 0; j < num_matches; j++) {
                        free(matches[j]);
                    }
                    free(matches);
                }
            } else {
                args[i++] = token;
            }
            token = strtok(NULL, " ");
        }
        args[i] = NULL;  // Terminer le tableau d'arguments par NULL

        // Gestion des commandes spécifiques
        if (strcmp(args[0], "myls") == 0) {
            myls_run(i, args);
            exit(0);
        } else if (strcmp(args[0], "myps") == 0) {
            myps();
            exit(0);
        } if (strncmp(command, "echo ", 5) == 0) {
            char *args = command + 5;
            substitute_variables(args); // Traiter les variables si elles existent
            printf("%s\n", args);
            return 0; // Succès
        } else {
            // Exécuter la commande avec execvp
            if (execvp(args[0], args) == -1) {
                fprintf(stderr, "Command not found: %s\n", args[0]);
                exit(127);  // État de sortie non nul en cas d'échec
            }
        }
    } else if (pid > 0) {

        if (background) {
            add_job(pid, command); // Ajouter le processus à la liste des jobs
        } else {
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid failed");
                return 127;  // Indiquer un échec
            }
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);  // Retourner le code de sortie de l'enfant
            }
        }
        return 127;  // Retourner une valeur non nulle si l'enfant ne s'est pas terminé normalement
    } else {
        perror("fork failed");
    }
    return 127;  // Retourner une valeur non nulle si fork échoue
}

// Commande myjobs
void execute_myjobs() {
    list_jobs();
}

// Commandes myfg et mybg
void execute_myfg(int job_id) {
    bring_job_to_foreground(job_id);
}

void execute_mybg(int job_id) {
    move_job_to_background(job_id);
}

// Fonction pour remplacer les variables dans une commande
void substitute_variables(char *command) {
    char buffer[1024];
    char *src = command, *dest = buffer;

    while (*src) {
        if (*src == '$') {
            src++; // Passer le symbole '$'
            char var_name[64];
            char *var_start = var_name;

            // Lire le nom de la variable
            while (*src && ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || *src == '_')) {
                *var_start++ = *src++;
            }
            *var_start = '\0';

            // Récupérer la valeur de la variable
            char *value = get_variable_value(var_name);
            if (value) {
                while (*value) {
                    *dest++ = *value++;
                }
            } else {
                fprintf(stderr, "Variable not defined: $%s\n", var_name);
            }
        } else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';

    // Copier le résultat dans la commande d'origine
    strncpy(command, buffer, sizeof(buffer) - 1);
    command[sizeof(buffer) - 1] = '\0';
}

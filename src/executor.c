#include "../include/executor.h"
#include "../include/myls.h"
#include "../include/myps.h"
#include "../include/parser.h"
#include "../include/wildcard.h"
#include "../include/redirection.h"
#include "../include/process_manager.h"
#include "../include/variable.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>


int background = 0;



/**
 * @brief Exécute une commande avec ou sans arguments.
 * 
 * @param command La commande à exécuter, incluant éventuellement des arguments.
 * @return int Le code de retour de la commande après son exécution.
 * 
 */
int execute_command(char *command) {

    
    if (strchr(command, '|')) {
        return handle_pipeline(command);
    }

   
    char *ampersand = strrchr(command, '&');
    if (ampersand && *(ampersand + 1) == '\0') {
        background = 1;
        *ampersand = '\0'; 
        while (*(ampersand - 1) == ' ') {
            *(--ampersand) = '\0'; 
        }
    } else {
        background = 0;
    }

    pid_t pid = fork();

    if (pid == 0) {
        
        handle_redirection(command);

        char *args[100];
        int i = 0;

        char *token = strtok(command, " ");
        while (token != NULL && i < 99) {
            int has_wildcard = (strchr(token, '*') != NULL || strchr(token, '?') != NULL || strchr(token, '[') != NULL);
            int is_escaped_token = 0;

             remove_quotes(token);

            
            for (int j = 0; token[j] != '\0'; j++) {
                if (is_escaped(token, j)) {
                    is_escaped_token = 1;
                    break;
                }
            }

            
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
        args[i] = NULL;

        
        if (strcmp(args[0], "myls") == 0) {
            myls_run(i, args);
            exit(0);
        } else {
            if (execvp(args[0], args) == -1) {
                fprintf(stderr, "Command not found: %s\n", args[0]);
                exit(127);
            }
        }
    } else if (pid > 0) {
        if (background) {
            add_job(pid, command); 
            printf("[%d] %d\n", job_count, pid);
        } else {
            int status;
            if (waitpid(pid, &status, 0) == -1) {
                perror("waitpid failed");
                return 127;
            }
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
        }
    } else {
        perror("fork failed");
        return 127;
    }
    return 0;
}


/**
 * @brief Affiche la liste des tâches en cours d'exécution en arrière-plan.
 * 
 * Cette fonction affiche les processus qui ont été lancés en mode arrière-plan 
 * en utilisant les fonctionnalités de gestion des tâches du shell.
 * 
 */
void execute_myjobs() {
    list_jobs();
}

/**
 * @brief Amène une tâche en arrière-plan au premier plan.
 * 
 * @param job_id L'identifiant de la tâche à ramener au premier plan.
 * Cette fonction suspend l'exécution du shell jusqu'à la fin de la tâche.
 * 
 */
void execute_myfg(int job_id) {
    bring_job_to_foreground(job_id);
}


/**
 * @brief Relance une tâche suspendue ou arrêtée en arrière-plan.
 * 
 * @param job_id L'identifiant de la tâche à relancer en arrière-plan.
 * Cette fonction rétablit l'exécution d'une tâche arrêtée sans la ramener au premier plan.
 * 
 */
void execute_mybg(int job_id) {
    move_job_to_background(job_id);
}



/**
 * @brief Remplace les variables dans une commande par leurs valeurs correspondantes.
 * 
 * Cette fonction analyse une commande donnée pour identifier les variables représentées par `$<nom>`.
 * Elle remplace chaque variable trouvée par sa valeur associée, si elle est définie. Si une variable
 * n'est pas définie, un message d'erreur est affiché.
 * 
 * @param command La commande contenant potentiellement des variables à remplacer. 
 *        La commande d'origine est modifiée en place pour refléter les substitutions.
 */
void substitute_variables(char *command) {
    char buffer[1024];
    char *src = command, *dest = buffer;

    while (*src) {
        if (*src == '$') {
            src++;
            char var_name[64] = {0};
            char *var_start = var_name;

            while (*src && isalnum(*src) && (var_start - var_name) < (int)sizeof(var_name) - 1) {
                *var_start++ = *src++;
            }
            *var_start = '\0';

            char *value = get_variable_value(var_name);
            if (value) {
                size_t value_len = strlen(value);
                size_t remaining_space = sizeof(buffer) - (dest - buffer) - 1;
                if (value_len > remaining_space) {
                    fprintf(stderr, "Variable substitution exceeds buffer size: $%s\n", var_name);
                    return;
                }
                strncpy(dest, value, remaining_space);
                dest += value_len;
            } else {
                fprintf(stderr, "Variable not defined: $%s\n", var_name);
            }
        } else {
            if ((dest - buffer) >= (int)sizeof(buffer) - 1) {
                fprintf(stderr, "Buffer overflow detected during substitution.\n");
                return;
            }
            *dest++ = *src++;
        }
    }
    *dest = '\0';

    strncpy(command, buffer, sizeof(buffer) - 1);
    command[sizeof(buffer) - 1] = '\0';
}

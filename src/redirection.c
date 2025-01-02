#include "../include/redirection.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>



/**
 * @brief Gère les redirections d'entrée, de sortie et des erreurs standard dans une commande.
 * 
 * Cette fonction identifie et configure les redirections pour une commande, 
 * notamment les redirections de type `>`, `>>`, `<` et `2>`.
 * Elle modifie les descripteurs de fichiers en conséquence pour diriger 
 * les entrées/sorties vers les fichiers spécifiés.
 * 
 * @param command La commande contenant les instructions de redirection.
 */
void handle_redirection(char *command) {
    int fd;
    char *error_redirect = strstr(command, "2>");
    if (error_redirect) {
        int append = (*(error_redirect + 2) == '>'); 
        *error_redirect = '\0'; 
        error_redirect += append ? 3 : 2;

        while (*error_redirect == ' ') error_redirect++;

        fd = open(error_redirect, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
        if (fd == -1) {
            perror("open failed");
            return;
        }
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    
    char *output_redirect = strstr(command, ">");
    if (output_redirect && *(output_redirect - 1) != '2') {
        int append = (*(output_redirect + 1) == '>');
        *output_redirect = '\0'; 
        output_redirect += append ? 2 : 1;

        while (*output_redirect == ' ') output_redirect++;

        fd = open(output_redirect, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
        if (fd == -1) {
            perror("open failed");
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    
    char *input_redirect = strstr(command, "<");
    if (input_redirect) {
        *input_redirect = '\0'; 
        input_redirect++;

        while (*input_redirect == ' ') input_redirect++;

        fd = open(input_redirect, O_RDONLY);
        if (fd == -1) {
            perror("open failed");
            return;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
}



/**
 * @brief Gère l'exécution de commandes en pipeline.
 * 
 * Cette fonction divise une commande en plusieurs sous-commandes 
 * reliées par des pipes (`|`). Chaque sous-commande est exécutée dans 
 * un processus enfant, et les entrées/sorties sont chaînées 
 * via des descripteurs de pipe.
 * 
 * @param command La commande complète avec des sous-commandes séparées par `|`.
 * @return int Le code de retour de la commande (0 en cas de succès, 1 sinon).
 */
int handle_pipeline(char *command) {
    char *commands[10];
    int num_commands = 0;

    
    char *token = strtok(command, "|");
    while (token != NULL && num_commands < 10) {
        commands[num_commands++] = token;
        token = strtok(NULL, "|");
    }

    int pipefd[2], in_fd = 0;
    pid_t pid;

    for (int i = 0; i < num_commands; i++) {
        if (pipe(pipefd) == -1) {
            perror("pipe failed");
            return 1;
        }

        if ((pid = fork()) == -1) {
            perror("fork failed");
            return 1;
        }

        if (pid == 0) { 
            dup2(in_fd, STDIN_FILENO);
            if (i < num_commands - 1) {
                dup2(pipefd[1], STDOUT_FILENO);
            }
            close(pipefd[0]);
            handle_redirection(commands[i]);

            char *args[100];
            int j = 0;
            char *arg = strtok(commands[i], " ");
            while (arg != NULL) {
                args[j++] = arg;
                arg = strtok(NULL, " ");
            }
            args[j] = NULL;

            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        } else { 
            wait(NULL);
            close(pipefd[1]);
            in_fd = pipefd[0]; 
        }
    }

    return 0;
}

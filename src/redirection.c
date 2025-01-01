#include "../include/redirection.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>


void handle_redirection(char *command) {
    int fd;

    // Redirection pour '2>' ou '2>>'
    char *error_redirect = strstr(command, "2>");
    if (error_redirect) {
        int append = (*(error_redirect + 2) == '>'); // Vérifie si c'est '2>>'
        *error_redirect = '\0'; // Sépare la commande avant '2>'
        error_redirect += append ? 3 : 2; // Passe '2>' ou '2>>'

        while (*error_redirect == ' ') error_redirect++; // Ignore les espaces

        fd = open(error_redirect, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
        if (fd == -1) {
            perror("open failed");
            return;
        }
        dup2(fd, STDERR_FILENO); // Redirige stderr (flux 2)
        close(fd);
    }

    // Redirection pour '>' ou '>>'
    char *output_redirect = strstr(command, ">");
    if (output_redirect && *(output_redirect - 1) != '2') { // Évite '2>' traité plus haut
        int append = (*(output_redirect + 1) == '>');
        *output_redirect = '\0'; // Sépare la commande avant '>'
        output_redirect += append ? 2 : 1; // Passe '>' ou '>>'

        while (*output_redirect == ' ') output_redirect++; // Ignore les espaces

        fd = open(output_redirect, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
        if (fd == -1) {
            perror("open failed");
            return;
        }
        dup2(fd, STDOUT_FILENO); // Redirige stdout (flux 1)
        close(fd);
    }

    // Redirection pour '<'
    char *input_redirect = strstr(command, "<");
    if (input_redirect) {
        *input_redirect = '\0'; // Sépare la commande avant '<'
        input_redirect++; // Passe '<'

        while (*input_redirect == ' ') input_redirect++; // Ignore les espaces

        fd = open(input_redirect, O_RDONLY);
        if (fd == -1) {
            perror("open failed");
            return;
        }
        dup2(fd, STDIN_FILENO); // Redirige stdin (flux 0)
        close(fd);
    }
}


int handle_pipeline(char *command) {
    char *commands[10];
    int num_commands = 0;

    // Split commands by '|'
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

        if (pid == 0) { // Child process
            dup2(in_fd, STDIN_FILENO); // Read from the previous pipe
            if (i < num_commands - 1) {
                dup2(pipefd[1], STDOUT_FILENO); // Write to the next pipe
            }
            close(pipefd[0]);
            handle_redirection(commands[i]); // Handle redirections within the command

            char *args[100];
            int j = 0;
            char *arg = strtok(commands[i], " ");
            while (arg != NULL) {
                args[j++] = arg;
                arg = strtok(NULL, " ");
            }
            args[j] = NULL;

            execvp(args[0], args); // Execute the command
            perror("execvp failed");
            exit(1);
        } else { // Parent process
            wait(NULL); // Wait for the child to finish
            close(pipefd[1]);
            in_fd = pipefd[0]; // Save the read end for the next command
        }
    }

    return 0;
}

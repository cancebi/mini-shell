#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int execute_command(char *command) {
    pid_t pid = fork();
    int status;

    if (pid == 0) {
        // Child process: execute the command with arguments
        char *args[100];
        int i = 0;

        // Split the command into tokens
        char *token = strtok(command, " ");
        while (token != NULL && i < 99) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;  // Null-terminate the args array

        if (execvp(args[0], args) == -1) {
            perror("execvp failed");
            exit(127);  // Non-zero exit status for failure
        }
    } else if (pid > 0) {
        // Parent process: wait for the child to complete
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);  // Return the child's exit status
        }
    } else {
        perror("fork failed");
    }
    return 127;  // Non-zero exit status if fork fails
}

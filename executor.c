#include "executor.h"
#include "wildcard.h"
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
        char *token = strtok(command, " "); // Splitting by space
        while (token != NULL && i < 99) {
            if (strpbrk(token, "*?") != NULL) {
                int num_matches = 0;
                char **matches = expand_wildcard(token, &num_matches);
                if (matches) {
                    for (int j = 0; j < num_matches && i < 99; j++) {
                        args[i++] = matches[j];
                    }
                    free(matches);
                }
            } else {
                args[i++] = token;
            }
            token = strtok(NULL, " ");
        }
        args[i] = NULL;  // Null-terminate the args array

        if (execvp(args[0], args) == -1) {
            perror("execvp failed");
            exit(127);  // Non-zero exit status for failure
        }
    } else if (pid > 0) {
        // Parent process: wait for the child to complete
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            return 127;  // Indicate failure to execute
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);  // Return the child's exit status
        }
        return 127;  // Non-zero if child didn't terminate normally
    } else {
        perror("fork failed");
    }
    return 127;  // Return non-zero if fork fails
}

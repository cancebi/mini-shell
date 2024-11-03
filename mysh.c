#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "executor.h"
#include "parser.h"

#define MAX_COMMAND_LENGTH 100
#define MAX_PATH_LENGTH 1024

int change_directory(char *path) {
    // If `path` is NULL or "~", set it to the home directory
    if (path == NULL || strcmp(path, "~") == 0) {
        path = getenv("HOME");  // Default to home directory
    }

    // Attempt to change directory
    if (chdir(path) != 0) {
        perror("cd failed");
        return 1;  // Non-zero status for failure
    }
    return 0;  // Zero status for success
}

void run_shell() {
    char command_line[MAX_COMMAND_LENGTH];
    int num_commands;
    int last_status = 0;
    char current_directory[MAX_PATH_LENGTH];

    while (1) {
        if(getcwd(current_directory, sizeof(current_directory)) == NULL){
            perror("getcwd failed");
            strcpy(current_directory, "?"); 
        }
        printf("mysh:%s> ", current_directory);
        if (fgets(command_line, MAX_COMMAND_LENGTH, stdin) == NULL) {
            printf("Error reading command.\n");
            continue;
        }

        command_line[strcspn(command_line, "\n")] = 0; // Remove newline character

        if(strncmp(command_line, "cd", 2) == 0 ){
            char *directory = strtok(command_line + 3, " ");
            change_directory(directory);
            continue;
        }

        if (strcmp(command_line, "exit") == 0) {
            break;
        }

        ParsedCommand *commands = parse_input(command_line, &num_commands);

        for (int i = 0; i < num_commands; i++) {

            // Determine if the command should run
            bool should_run = false;

            // For the first command, always run it
            if (i == 0) {
                should_run = true;
            } 
            // For subsequent commands, check the previous command's status
            else if (commands[i - 1].condition == COND_SUCCESS) {
                should_run = (last_status == 0); // for &&
            }
            else if (commands[i - 1].condition == COND_FAILURE){
                should_run = (last_status != 0); //for ||
            }
            else{
                should_run = true;
            }


            // Execute the command if determined to run
            if (should_run) {
                last_status = execute_command(commands[i].command);
            } 
        }

        // Free allocated memory for commands
        for (int i = 0; i < num_commands; i++) {
            free(commands[i].command);
        }
        free(commands);
    }
}

int main() {
    run_shell();
    return 0;
}

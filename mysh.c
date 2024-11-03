#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "executor.h"
#include "parser.h"

#define MAX_COMMAND_LENGTH 100

void run_shell() {
    char command_line[MAX_COMMAND_LENGTH];
    int num_commands;
    int last_status = 0;

    while (1) {
        printf("mysh> ");
        if (fgets(command_line, MAX_COMMAND_LENGTH, stdin) == NULL) {
            printf("Error reading command.\n");
            continue;
        }

        command_line[strcspn(command_line, "\n")] = 0; // Remove newline character

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
                should_run = (last_status == 0);
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

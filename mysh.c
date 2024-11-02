// mysh.c
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

        // Remove newline character
        command_line[strcspn(command_line, "\n")] = '\0';

        // If the user types "exit", break out of the loop
        if (strcmp(command_line, "exit") == 0) {
            break;
        }

        // Parse the command line into individual commands
        ParsedCommand *commands = parse_input(command_line, &num_commands);

        // Execute each parsed command based on conditions
        for (int i = 0; i < num_commands; i++) {
            printf("[DEBUG] Last status before command %d ('%s'): %d\n", i, commands[i].command, last_status);

            // Determine whether to execute the command based on last_status and the run_if_success flag
            if (i == 0 || 
                (commands[i].run_if_success && last_status == 0) ||  // Execute on success for &&
                (!commands[i].run_if_success && last_status != 0)) { // Execute on failure for ||

                    // Execute the command and update last_status
                last_status = execute_command(commands[i].command);

                printf("[DEBUG] Last status after command %d ('%s'): %d\n", i, commands[i].command, last_status);
            } else {
                // If the command is skipped, print a debug message
                printf("[DEBUG] Skipping command %d ('%s') based on last status: %d\n", i, commands[i].command, last_status);
            }
        }


        // Free memory for commands
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

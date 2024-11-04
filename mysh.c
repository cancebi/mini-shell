#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "executor.h"
#include "parser.h"

#define MAX_COMMAND_LENGTH 100
#define MAX_PATH_LENGTH 1024

volatile sig_atomic_t foreground_running = 0;
char last_command_name[MAX_COMMAND_LENGTH] = "";
int last_status = -1;

void handle_sigint(int sig){
    if (foreground_running){
        printf("\n[DEBUG] SIGINT received, passing to foreground process.\n");
        return;
    }
    printf("\nDo you want to quit mysh? (y/n): ");
    int response = getchar();
    getchar();
    if(response == 'y' || response == 'Y'){
        printf("Exiting mysh. \n");
        exit(0);
    }
    else{
        printf("Continuing mysh. \n");
    }
}

int change_directory(char *path) {
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

void print_status() {
    if (last_status == -1) {
        printf("No commands have been executed yet.\n");
    } else {
        if (last_status == 0) {
            printf("%s terminé avec comme code de retour %d\n", last_command_name, last_status);
        } else {
            printf("%s terminé anormalement\n", last_command_name);
        }
    }
    // Update to reflect that `status` was the last executed command
    strcpy(last_command_name, "status");
    last_status = 0;
}

void run_shell() {
    char command_line[MAX_COMMAND_LENGTH];
    int num_commands;
    char current_directory[MAX_PATH_LENGTH];

    signal(SIGINT, handle_sigint);

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
            last_status = change_directory(directory);
            strncpy(last_command_name, "cd", MAX_COMMAND_LENGTH);
            continue;
        }

        if (strcmp(command_line, "exit") == 0) {
            printf("Exiting mysh. \n");
            exit(0);
        }

        if (strcmp(command_line, "status") == 0) {
            print_status();
            continue;
        }

        ParsedCommand *commands = parse_input(command_line, &num_commands);

        for (int i = 0; i < num_commands; i++) {
            bool should_run = (i == 0) ||
                              (commands[i - 1].condition == COND_SUCCESS && last_status == 0) ||
                              (commands[i - 1].condition == COND_FAILURE && last_status != 0) ||
                              (commands[i - 1].condition == COND_ALWAYS);

            if (should_run) {
                foreground_running = 1;
                strncpy(last_command_name, commands[i].command, MAX_COMMAND_LENGTH);  // Store the command name
                last_status = execute_command(commands[i].command);  // Execute command and update status
                foreground_running = 0;
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "../include/executor.h"
#include "../include/parser.h"
#include "../include/variable.h"


#define ROUGE(x) "\033[31m" x "\033[0m"
#define VERT(x) "\033[32m" x "\033[0m"

#define MAX_COMMAND_LENGTH 100
#define MAX_PATH_LENGTH 1024

volatile sig_atomic_t foreground_running = 0;
char last_command_name[MAX_COMMAND_LENGTH] = "";
int last_status = -1;
char current_directory[MAX_PATH_LENGTH];  
char global_command_line[MAX_COMMAND_LENGTH];

void handle_sigint(int sig){
    if (foreground_running){
        return;
    }
    printf("\nDo you want to quit mysh? (y/n): ");
    fflush(stdout);

    int response = getchar();
    while (getchar() != '\n'); // Clear any additional input characters

    if (response == 'y' || response == 'Y') {
        printf("Exiting mysh.\n");
        exit(0);
    }
    else{
        printf("Continuing mysh. \n");
        if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
            perror("getcwd failed");
            strcpy(current_directory, "?");
        }
        printf("mysh:%s ~> ", current_directory);
        fflush(stdout);
    }
}

int change_directory(char *path) {
    if (path == NULL || strcmp(path, "~") == 0) {
        path = getenv("HOME");  // Default to home directory
    }

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
            printf(VERT("%s terminé avec comme code de retour %d\n"), last_command_name, last_status);
        } else {
            printf(ROUGE("%s terminé anormalement\n"), last_command_name);
        }
    } 
    strcpy(last_command_name, "status");
    last_status = 0;
}

void run_shell() {
    int num_commands;

    signal(SIGINT, handle_sigint);

    while (1) {
        if(getcwd(current_directory, sizeof(current_directory)) == NULL) {
            perror("getcwd failed");
            strcpy(current_directory, "?"); 
        }
        printf("mysh:%s ~> ", current_directory);
        fflush(stdout);

        ssize_t n = read(STDIN_FILENO, global_command_line, MAX_COMMAND_LENGTH - 1);
        if (n > 0) {
            global_command_line[n] = '\0'; // Null-terminate the input
            size_t length = strcspn(global_command_line, "\n");
            global_command_line[length] = '\0'; // Remove newline if present
        } else if (n == -1) {
            perror("read error");
            continue;
        }

        if (strlen(global_command_line) == 0) {
            continue; // Skip to the next iteration if input is empty
        }

        if (strspn(global_command_line, " ") == strlen(global_command_line)) {
            continue; // Skip to the next iteration if input is only whitespace
        }

        if(strncmp(global_command_line, "cd", 2) == 0 ){
            char *directory = strtok(global_command_line + 3, " ");
            last_status = change_directory(directory);
            strncpy(last_command_name, "cd", MAX_COMMAND_LENGTH);
            continue;
        }

        if (strcmp(global_command_line, "exit") == 0) {
            printf("Exiting mysh.\n");
            exit(0);
        }

        if (strcmp(global_command_line, "status") == 0) {
            print_status();
            continue;
        }

        if (strncmp(global_command_line, "myjobs", 6) == 0) {
            execute_myjobs();
            continue;
        }
        
        if (strncmp(global_command_line, "myfg", 4) == 0) {
            int job_id = atoi(global_command_line + 5);
            execute_myfg(job_id);
            continue;
        }
        
        if (strncmp(global_command_line, "mybg", 4) == 0) {
            int job_id = atoi(global_command_line + 5);
            execute_mybg(job_id);
            continue;
        }

        if (strncmp(global_command_line, "set ", 4) == 0) {
            char *name = strtok(global_command_line + 4, "=");
            char *value = strtok(NULL, "");
            if (name && value) {
                set_local_variable(name, value);
            } else {
                fprintf(stderr, "Usage: set name=value\n");
            }
            continue;
        }

        if (strncmp(global_command_line, "setenv ", 7) == 0) {
            char *name = strtok(global_command_line + 7, "=");
            char *value = strtok(NULL, "");
            if (name && value) {
                set_env_variable(name, value);
            } else {
                fprintf(stderr, "Usage: setenv name=value\n");
            }
            continue;
        }



        if (strncmp(global_command_line, "unset ", 6) == 0) {
            char *name = global_command_line + 6;
            // Ne pas interpréter $ comme une substitution dans unset
            if (name[0] == '$') {
                name++; // Ignorer le symbole $ pour obtenir le nom réel de la variable
            }
            unset_local_variable(name);
            continue;
        }



        if (strncmp(global_command_line, "unsetenv ", 9) == 0) {
            char *name = global_command_line + 9;
            // Ne pas interpréter $ comme une substitution dans unsetenv
            if (name[0] == '$') {
                name++; // Ignorer le symbole $ pour obtenir le nom réel de la variable
            }
            unset_env_variable(name);
            continue;
        }
        
        
        if (strncmp(global_command_line, "echo ", 5) == 0) {
            char *args = global_command_line + 5;
            substitute_variables(args); // Traiter les variables si elles existent
            printf("%s\n", args);
            continue;
        } 

        ParsedCommand *commands = parse_input(global_command_line, &num_commands);

        for (int i = 0; i < num_commands; i++) {
            bool should_run = (i == 0) ||
                              (commands[i - 1].condition == COND_SUCCESS && last_status == 0) ||
                              (commands[i - 1].condition == COND_FAILURE && last_status != 0) ||
                              (commands[i - 1].condition == COND_ALWAYS);

            if (should_run) {
                foreground_running = 1;
                strncpy(last_command_name, commands[i].command, MAX_COMMAND_LENGTH);
                last_status = execute_command(commands[i].command);
                foreground_running = 0;
            }
        }

        for (int i = 0; i < num_commands; i++) {
            free(commands[i].command);
        }
        free(commands);
    }
}

int main() {
     // Initialiser la mémoire partagée pour les variables d'environnement
    init_shared_memory();

    // Lancer le shell
    run_shell();

    // Libérer la mémoire partagée à la fin
    destroy_shared_memory();
    return 0;
}

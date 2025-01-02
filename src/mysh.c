#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "../include/executor.h"
#include "../include/parser.h"
#include "../include/variable.h"
#include "../include/myps.h"


#define ROUGE(x) "\033[31m" x "\033[0m"
#define VERT(x) "\033[32m" x "\033[0m"

#define MAX_COMMAND_LENGTH 100
#define MAX_PATH_LENGTH 1024

volatile sig_atomic_t foreground_running = 0;
char last_command_name[MAX_COMMAND_LENGTH] = "";
int last_status = -1;
char current_directory[MAX_PATH_LENGTH];  
char global_command_line[MAX_COMMAND_LENGTH];


/**
 * @brief Gère le signal SIGINT (Ctrl+C) et propose de quitter le shell.
 * 
 * Si un processus en avant-plan est en cours, le signal est ignoré.
 * Sinon, l'utilisateur est invité à confirmer la sortie du shell.
 * 
 * @param sig Signal reçu (toujours SIGINT).
 */
void handle_sigint(int sig){
    if (foreground_running){
        return;
    }
    printf("\nDo you want to quit mysh? (y/n): ");
    fflush(stdout);

    int response = getchar();
    while (getchar() != '\n');

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


/**
 * @brief Change le répertoire de travail actuel.
 * 
 * Cette fonction met à jour le répertoire de travail vers le chemin spécifié.
 * Si le chemin est NULL ou "~", il change vers le répertoire HOME.
 * 
 * @param path Chemin vers le répertoire cible.
 * @return int 0 si réussi, 1 sinon.
 */
int change_directory(char *path) {
    if (path == NULL || strcmp(path, "~") == 0) {
        path = getenv("HOME");
    }

    if (chdir(path) != 0) {
        perror("cd failed");
        return 1; 
    }
    return 0;
}


/**
 * @brief Affiche le statut de la dernière commande exécutée.
 * 
 * Si aucune commande n'a été exécutée, un message approprié est affiché.
 * Indique également si la dernière commande s'est terminée correctement ou non.
 */
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


/**
 * @brief Boucle principale du shell interactif.
 * 
 * Gère la lecture des commandes utilisateur, l'analyse, et leur exécution.
 * Supporte les commandes internes telles que `cd`, `exit`, `status`, `myjobs`, 
 * ainsi que la gestion des variables locales et d'environnement.
 */
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
            global_command_line[n] = '\0';
            size_t length = strcspn(global_command_line, "\n");
            global_command_line[length] = '\0';
        } else if (n == -1) {
            perror("read error");
            continue;
        }

        if (strlen(global_command_line) == 0) {
            continue;
        }

        if (strspn(global_command_line, " ") == strlen(global_command_line)) {
            continue;
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
        
        if (strcmp(global_command_line, "myps") == 0) {
            myps();
            strncpy(last_command_name, "myps", MAX_COMMAND_LENGTH);
            last_status = 0;
            continue;
        }

        if (strcmp(global_command_line, "myjobs") == 0) {
            execute_myjobs();
            strncpy(last_command_name, "myjobs", MAX_COMMAND_LENGTH);
            last_status = 0;
            continue;
        }

        if (strncmp(global_command_line, "myfg", 4) == 0) {
            int job_id = atoi(global_command_line + 5);
            execute_myfg(job_id);
            strncpy(last_command_name, "myfg", MAX_COMMAND_LENGTH);
            last_status = 0;
            continue;
        }

        if (strncmp(global_command_line, "mybg", 4) == 0) {
            int job_id = atoi(global_command_line + 5);
            execute_mybg(job_id);
            strncpy(last_command_name, "mybg", MAX_COMMAND_LENGTH);
            last_status = 0;
            continue;
        }

        if (strncmp(global_command_line, "set ", 4) == 0) {
            char *name = strtok(global_command_line + 4, "=");
            char *value = strtok(NULL, "");
            if (name && value) {
                set_local_variable(name, value);
                strncpy(last_command_name, "set", MAX_COMMAND_LENGTH);
                last_status = 0;
            } else {
                fprintf(stderr, "Usage: set name=value\n");
                last_status = 1;
            }
            continue;
        }

        if (strncmp(global_command_line, "setenv ", 7) == 0) {
            char *name = strtok(global_command_line + 7, "=");
            char *value = strtok(NULL, "");
            if (name && value) {
                set_env_variable(name, value);
                strncpy(last_command_name, "setenv", MAX_COMMAND_LENGTH);
                last_status = 0;
            } else {
                fprintf(stderr, "Usage: setenv name=value\n");
                last_status = 1;
            }
            continue;
        }

        if (strncmp(global_command_line, "unset ", 6) == 0) {
            char *name = global_command_line + 6;
            if (name[0] == '$') {
                name++;
            }
            unset_local_variable(name);
            strncpy(last_command_name, "unset", MAX_COMMAND_LENGTH);
            last_status = 0;
            continue;
        }

        if (strncmp(global_command_line, "unsetenv ", 9) == 0) {
            char *name = global_command_line + 9;
            if (name[0] == '$') {
                name++; 
            }
            unset_env_variable(name);
            strncpy(last_command_name, "unsetenv", MAX_COMMAND_LENGTH);
            last_status = 0;
            continue;
        }

        if (strncmp(global_command_line, "echo ", 5) == 0) {
            char *args = global_command_line + 5;
            substitute_variables(args);
            printf("%s\n", args);
            strncpy(last_command_name, "echo", MAX_COMMAND_LENGTH);
            last_status = 0;
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


/**
 * @brief Point d'entrée principal du programme.
 * 
 * Initialise les ressources nécessaires telles que la mémoire partagée,
 * démarre la boucle principale du shell et nettoie les ressources à la fin.
 * 
 * @return int Code de retour du programme.
 */
int main() {

    // Initialiser la mémoire partagée pour les variables d'environnement
    init_shared_memory();

    // Lancer le shell
    run_shell();

    // Libérer la mémoire partagée à la fin
    destroy_shared_memory();

    return 0;
}

#include "variable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>

// Structures pour les variables locales
typedef struct LocalVariable {
    char name[64];
    char value[256];
    struct LocalVariable *next;
} LocalVariable;

LocalVariable *local_vars = NULL;

// Structures pour les variables d'environnement
#define SHM_KEY 12345
#define SHM_SIZE 4096
char *shared_memory = NULL;
int shm_id;
pthread_mutex_t shm_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialisation de la mémoire partagée
void init_shared_memory() {
    shm_id = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        exit(1);
    }
    shared_memory = (char *)shmat(shm_id, NULL, 0);
    if (shared_memory == (char *)-1) {
        perror("shmat failed");
        exit(1);
    }
    memset(shared_memory, 0, SHM_SIZE);
}

// Destruction de la mémoire partagée
void destroy_shared_memory() {
    if (shmdt(shared_memory) == -1) {
        perror("shmdt failed");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
    }
}

// Gestion des variables locales
void set_local_variable(const char *name, const char *value) {
    if (!name || !value || strlen(name) == 0 || strlen(value) == 0 || strlen(name) >= 64 || strlen(value) >= 256) {
        fprintf(stderr, "Error: Invalid name or value for local variable.\n");
        return;
    }

    LocalVariable *current = local_vars;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            // Mise à jour si la variable existe
            strncpy(current->value, value, sizeof(current->value) - 1);
            current->value[sizeof(current->value) - 1] = '\0';
            return;
        }
        current = current->next;
    }

    // Ajouter une nouvelle variable
    LocalVariable *new_var = malloc(sizeof(LocalVariable));
    if (!new_var) {
        perror("malloc failed");
        return;
    }

    memset(new_var, 0, sizeof(LocalVariable)); // Initialiser la structure
    strncpy(new_var->name, name, sizeof(new_var->name) - 1);
    new_var->name[sizeof(new_var->name) - 1] = '\0';
    strncpy(new_var->value, value, sizeof(new_var->value) - 1);
    new_var->value[sizeof(new_var->value) - 1] = '\0';
    new_var->next = local_vars;
    local_vars = new_var;
}


void unset_local_variable(const char *name) {
    if (!name || strlen(name) == 0) {
        fprintf(stderr, "Error: Invalid name for local variable.\n");
        return;
    }

    LocalVariable **current = &local_vars;
    while (*current) {
        if (strcmp((*current)->name, name) == 0) {
            LocalVariable *to_delete = *current;
            *current = (*current)->next; // Réorganiser la liste
            free(to_delete); // Libérer la mémoire
            return;
        }
        current = &((*current)->next);
    }

    // Si la variable n'est pas trouvée
    fprintf(stderr, "Error: Variable %s not found.\n", name);
}



void list_local_variables() {
    LocalVariable *current = local_vars;
    while (current) {
        printf("%s=%s\n", current->name, current->value);
        current = current->next;
    }
}

// Gestion des variables d'environnement
void set_env_variable(const char *name, const char *value) {
    if (!shared_memory) {
        fprintf(stderr, "Shared memory not initialized.\n");
        return;
    }

    if (!name || !value || strlen(name) == 0 || strlen(value) == 0 || strlen(name) >= 64 || strlen(value) >= 256) {
        fprintf(stderr, "Error: Invalid name or value for environment variable.\n");
        return;
    }

    pthread_mutex_lock(&shm_mutex);

    char *current = shared_memory;
    while (*current && current < shared_memory + SHM_SIZE - 256) {
        current += strlen(current) + 1;
    }

    if (current >= shared_memory + SHM_SIZE - 256) {
        fprintf(stderr, "Shared memory is full.\n");
        pthread_mutex_unlock(&shm_mutex);
        return;
    }

    snprintf(current, 256, "%s=%s", name, value);

    pthread_mutex_unlock(&shm_mutex);
}




void unset_env_variable(const char *name) {
    if (!shared_memory) {
        fprintf(stderr, "Shared memory not initialized.\n");
        return;
    }

    if (!name || strlen(name) == 0) {
        fprintf(stderr, "Error: Invalid name for environment variable.\n");
        return;
    }

    pthread_mutex_lock(&shm_mutex);

    char *current = shared_memory;
    int found = 0;
    while (*current) {
        if (strncmp(current, name, strlen(name)) == 0 && current[strlen(name)] == '=') {
            char *next = current + strlen(current) + 1;
            memmove(current, next, shared_memory + SHM_SIZE - next);
            found = 1;
            break;
        }
        current += strlen(current) + 1;
    }

    pthread_mutex_unlock(&shm_mutex);

    if (!found) {
        fprintf(stderr, "Error: Environment variable %s not found.\n", name);
    }
}


void list_env_variables() {
    char *current = shared_memory;
    while (*current) {
        printf("%s\n", current);
        current += strlen(current) + 1;
    }
}

// Récupérer la valeur d'une variable (locale ou environnement)
char *get_variable_value(const char *name) {
    if (!name || strlen(name) == 0) {
        return NULL; // Nom invalide
    }

    // Vérifier les variables locales
    LocalVariable *current = local_vars;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }

    // Vérifier les variables d'environnement
    char *current_env = shared_memory;
    while (*current_env) {
        if (strncmp(current_env, name, strlen(name)) == 0 && current_env[strlen(name)] == '=') {
            return current_env + strlen(name) + 1;
        }
        current_env += strlen(current_env) + 1;
    }

    return NULL; // Variable non trouvée
}

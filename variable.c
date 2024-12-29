#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "variable.h"

#define MAX_VARIABLES 100
#define SHARED_MEMORY_NAME "/mysh_env"
#define SHARED_MEMORY_SIZE 4096

// Variables locales
typedef struct {
    char *name;
    char *value;
} LocalVariable;

LocalVariable local_variables[MAX_VARIABLES];
int local_var_count = 0;

// Mémoire partagée
char *shared_memory = NULL;

// Variables locales
void set_local_variable(const char *name, const char *value) {
    for (int i = 0; i < local_var_count; i++) {
        if (strcmp(local_variables[i].name, name) == 0) {
            free(local_variables[i].value);
            local_variables[i].value = strdup(value);
            return;
        }
    }
    if (local_var_count < MAX_VARIABLES) {
        local_variables[local_var_count].name = strdup(name);
        local_variables[local_var_count].value = strdup(value);
        local_var_count++;
    } else {
        fprintf(stderr, "Maximum number of local variables reached\n");
    }
}

const char *get_local_variable(const char *name) {
    for (int i = 0; i < local_var_count; i++) {
        if (strcmp(local_variables[i].name, name) == 0) {
            return local_variables[i].value;
        }
    }
    return NULL;
}

void unset_local_variable(const char *name) {
    for (int i = 0; i < local_var_count; i++) {
        if (strcmp(local_variables[i].name, name) == 0) {
            free(local_variables[i].name);
            free(local_variables[i].value);
            local_variables[i] = local_variables[local_var_count - 1];
            local_var_count--;
            return;
        }
    }
}

// Variables d'environnement
void set_env_variable(const char *name, const char *value) {
    if (setenv(name, value, 1) != 0) {
        perror("setenv failed");
    }
}

const char *get_env_variable(const char *name) {
    return getenv(name);
}

void unset_env_variable(const char *name) {
    if (unsetenv(name) != 0) {
        perror("unsetenv failed");
    }
}

// Résolution des variables
const char *resolve_variable(const char *name) {
    const char *value = get_local_variable(name);
    if (value) {
        return value;
    }
    return get_env_variable(name);
}

// Gestion de la mémoire partagée
void initialize_shared_memory() {
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SHARED_MEMORY_SIZE);
    shared_memory = mmap(0, SHARED_MEMORY_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap failed");
        exit(EXIT_FAILURE);
    }
    close(shm_fd);
    memset(shared_memory, 0, SHARED_MEMORY_SIZE); // Initialiser avec des zéros
}

void cleanup_shared_memory() {
    if (munmap(shared_memory, SHARED_MEMORY_SIZE) == -1) {
        perror("munmap failed");
    }
    if (shm_unlink(SHARED_MEMORY_NAME) == -1) {
        perror("shm_unlink failed");
    }
}

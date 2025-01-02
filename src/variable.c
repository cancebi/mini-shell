#include "../include/variable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <pthread.h>


typedef struct LocalVariable {
    char name[64];
    char value[256];
    struct LocalVariable *next;
} LocalVariable;

LocalVariable *local_vars = NULL;


#define SHM_KEY 12345
#define SHM_SIZE 4096
char *shared_memory = NULL;
int shm_id;
pthread_mutex_t shm_mutex = PTHREAD_MUTEX_INITIALIZER;


/**
 * @brief Initialise la mémoire partagée pour stocker les variables d'environnement.
 * 
 * Cette fonction crée un segment de mémoire partagée avec une taille prédéfinie
 * et initialise son contenu à zéro. Elle est utilisée pour gérer les variables
 * d'environnement entre différents processus.
 */
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


/**
 * @brief Détruit la mémoire partagée utilisée pour les variables d'environnement.
 * 
 * Cette fonction détache le segment de mémoire partagée du processus actuel
 * et le marque pour suppression.
 */
void destroy_shared_memory() {
    if (shmdt(shared_memory) == -1) {
        perror("shmdt failed");
    }
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
    }
}


/**
 * @brief Définit une variable locale avec un nom et une valeur.
 * 
 * Cette fonction ajoute une nouvelle variable locale ou met à jour la valeur
 * d'une variable existante. Les noms et les valeurs ont des longueurs maximales
 * définies pour éviter les dépassements de mémoire.
 * 
 * @param name Le nom de la variable locale.
 * @param value La valeur de la variable locale.
 */
void set_local_variable(const char *name, const char *value) {
    
    if (!name || !value || strlen(name) == 0 || strlen(value) == 0 || strlen(name) >= 64 || strlen(value) >= 256) {
        fprintf(stderr, "Error: Invalid name or value for local variable.\n");
        return;
    }

    LocalVariable *current = local_vars;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            strncpy(current->value, value, sizeof(current->value) - 1);
            current->value[sizeof(current->value) - 1] = '\0';
            return;
        }
        current = current->next;
    }

    LocalVariable *new_var = malloc(sizeof(LocalVariable));
    if (!new_var) {
        perror("malloc failed");
        return;
    }

    memset(new_var, 0, sizeof(LocalVariable));
    strncpy(new_var->name, name, sizeof(new_var->name) - 1);
    new_var->name[sizeof(new_var->name) - 1] = '\0';
    strncpy(new_var->value, value, sizeof(new_var->value) - 1);
    new_var->value[sizeof(new_var->value) - 1] = '\0';
    new_var->next = local_vars;
    local_vars = new_var;
}


/**
 * @brief Supprime une variable locale existante.
 * 
 * Cette fonction recherche et supprime une variable locale par son nom.
 * Si la variable n'est pas trouvée, un message d'erreur est affiché.
 * 
 * @param name Le nom de la variable locale à supprimer.
 */
void unset_local_variable(const char *name) {
    if (!name || strlen(name) == 0) {
        fprintf(stderr, "Error: Invalid name for local variable.\n");
        return;
    }

    LocalVariable **current = &local_vars;
    while (*current) {
        if (strcmp((*current)->name, name) == 0) {
            LocalVariable *to_delete = *current;
            *current = (*current)->next; 
            free(to_delete);
            return;
        }
        current = &((*current)->next);
    }

    fprintf(stderr, "Error: Variable %s not found.\n", name);
}


/**
 * @brief Affiche toutes les variables locales et leurs valeurs.
 * 
 * Parcourt la liste des variables locales et imprime leurs noms et valeurs
 * au format `nom=valeur`.
 */
void list_local_variables() {
    LocalVariable *current = local_vars;
    while (current) {
        printf("%s=%s\n", current->name, current->value);
        current = current->next;
    }
}

/**
 * @brief Définit une variable d'environnement dans la mémoire partagée.
 * 
 * Cette fonction ajoute ou met à jour une variable d'environnement dans la mémoire partagée.
 * Si la mémoire est pleine ou non initialisée, elle affiche un message d'erreur.
 * 
 * @param name Le nom de la variable d'environnement.
 * @param value La valeur de la variable d'environnement.
 */
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



/**
 * @brief Supprime une variable d'environnement de la mémoire partagée.
 * 
 * Cette fonction recherche une variable d'environnement dans la mémoire partagée
 * et la supprime si elle existe.
 * 
 * @param name Le nom de la variable d'environnement à supprimer.
 */
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

/**
 * @brief Affiche toutes les variables d'environnement stockées dans la mémoire partagée.
 * 
 * Parcourt la mémoire partagée et imprime chaque variable d'environnement au format
 * `nom=valeur`.
 */
void list_env_variables() {
    char *current = shared_memory;
    while (*current) {
        printf("%s\n", current);
        current += strlen(current) + 1;
    }
}

/**
 * @brief Récupère la valeur d'une variable locale ou d'environnement.
 * 
 * Cette fonction recherche d'abord la variable dans la liste des variables locales,
 * puis dans les variables d'environnement stockées dans la mémoire partagée.
 * 
 * @param name Le nom de la variable à rechercher.
 * @return char* La valeur de la variable ou `NULL` si elle n'est pas trouvée.
 */
char *get_variable_value(const char *name) {
    if (!name || strlen(name) == 0) {
        return NULL;
    }

    
    LocalVariable *current = local_vars;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }

    char *current_env = shared_memory;
    while (*current_env) {
        if (strncmp(current_env, name, strlen(name)) == 0 && current_env[strlen(name)] == '=') {
            return current_env + strlen(name) + 1;
        }
        current_env += strlen(current_env) + 1;
    }

    return NULL;
}

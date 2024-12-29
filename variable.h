#ifndef VARIABLES_H
#define VARIABLES_H

// Fonctions pour les variables locales
void set_local_variable(const char *name, const char *value);
const char *get_local_variable(const char *name);
void unset_local_variable(const char *name);

// Fonctions pour les variables d'environnement
void set_env_variable(const char *name, const char *value);
const char *get_env_variable(const char *name);
void unset_env_variable(const char *name);

// Résolution des variables (priorité locale > environnement)
const char *resolve_variable(const char *name);

// Mémoire partagée pour les variables d'environnement
void initialize_shared_memory();
void cleanup_shared_memory();

#endif // VARIABLES_H

#ifndef VARIABLES_H
#define VARIABLES_H

void init_shared_memory();
void destroy_shared_memory();
void set_local_variable(const char *name, const char *value);
void unset_local_variable(const char *name);
void list_local_variables();
void set_env_variable(const char *name, const char *value);
void unset_env_variable(const char *name);
void list_env_variables();
char *get_variable_value(const char *name);
void save_shared_memory();
void load_shared_memory();

#endif // VARIABLES_H

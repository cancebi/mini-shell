#ifndef MYSH_H
#define MYSH_H

#include <sys/types.h>

/**
 * @brief Gère le signal SIGCHLD pour détecter les processus terminés.
 * 
 * @param sig Signal reçu.
 */
void handle_sigchld(int sig);

/**
 * @brief Gère le signal SIGTSTP pour arrêter un processus en avant-plan.
 * 
 * @param sig Signal reçu.
 */
void handle_sigtstp(int sig);

/**
 * @brief Boucle principale du shell.
 */
void run_shell();

/**
 * @brief Change le répertoire de travail.
 * 
 * @param path Chemin du nouveau répertoire.
 * @return int 0 si réussi, 1 sinon.
 */
int change_directory(char *path);

/**
 * @brief Affiche le statut de la dernière commande exécutée.
 */
void print_status();

#endif // MYSH_H

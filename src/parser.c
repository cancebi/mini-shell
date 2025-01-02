#include "../include/parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


/**
 * @brief Supprime les espaces au début et à la fin d'une chaîne de caractères.
 * 
 * Cette fonction modifie la chaîne donnée en place pour retirer tout espace
 * ou caractère blanc superflu au début et à la fin.
 * 
 * @param str Chaîne de caractères à nettoyer.
 */
void trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
}


/**
 * @brief Analyse une chaîne d'entrée en commandes distinctes avec leurs conditions.
 * 
 * Cette fonction découpe une chaîne d'entrée utilisateur en plusieurs commandes
 * basées sur les séparateurs `&&`, `||`, ou `;`, et attribue une condition d'exécution
 * pour chaque commande. Elle retourne un tableau de commandes analysées.
 * 
 * @param input Chaîne d'entrée utilisateur.
 * @param num_commands Pointeur pour stocker le nombre de commandes analysées.
 * @return ParsedCommand* Tableau de commandes analysées.
 */
ParsedCommand *parse_input(char *input, int *num_commands) {
    *num_commands = 0;
    int capacity = 10;
    ParsedCommand *commands = malloc(capacity * sizeof(ParsedCommand));
    char *token;
    char *sep;

    while (*input != '\0') {
        ParsedCommand cmd;

        // Default condition
        cmd.condition = COND_ALWAYS;

        // Check for `&&`, `||`, or `;`
        if ((sep = strstr(input, "&&")) != NULL) {
            *sep = '\0';
            cmd.condition = COND_SUCCESS;
            token = input;
            input = sep + 2;
        } else if ((sep = strstr(input, "||")) != NULL) {
            *sep = '\0';
            cmd.condition = COND_FAILURE;
            token = input;
            input = sep + 2;
        } else if ((sep = strchr(input, ';')) != NULL) {
            *sep = '\0';
            cmd.condition = COND_ALWAYS;
            token = input;
            input = sep + 1;
        } else {
            token = input;
            input += strlen(input);
        }

        trim_whitespace(token);
        cmd.command = strdup(token);

        if (*num_commands >= capacity) {
            capacity *= 2;
            commands = realloc(commands, capacity * sizeof(ParsedCommand));
        }
        commands[(*num_commands)++] = cmd;
    }
    
    return commands;
}


/**
 * @brief Supprime les guillemets simples ou doubles d'une chaîne.
 * 
 * Cette fonction modifie la chaîne donnée en place pour retirer tous les
 * guillemets simples (') ou doubles (") qu'elle contient.
 * 
 * @param str Chaîne de caractères à modifier.
 */
void remove_quotes(char *str) {
    char *src = str, *dest = str;

    while (*src) {
        if (*src != '"' && *src != '\'') {
            *dest++ = *src;
        }
        src++;
    }
    *dest = '\0';
}

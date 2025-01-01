#include "../include/parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

void trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
}

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
            cmd.condition = COND_SUCCESS;  // `&&` runs on success
            token = input;
            input = sep + 2;
        } else if ((sep = strstr(input, "||")) != NULL) {
            *sep = '\0';
            cmd.condition = COND_FAILURE; // `||` runs on failure
            token = input;
            input = sep + 2;
        } else if ((sep = strchr(input, ';')) != NULL) {
            *sep = '\0';
            cmd.condition = COND_ALWAYS; // `;` runs unconditionally
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

void remove_quotes(char *str) {
    char *src = str, *dest = str;

    while (*src) {
        if (*src != '"' && *src != '\'') { // Ignore les guillemets simples et doubles
            *dest++ = *src;
        }
        src++;
    }
    *dest = '\0'; // Terminer la chaîne
}

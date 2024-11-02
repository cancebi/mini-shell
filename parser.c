#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

void trim_whitespace(char *str) {
    char *end;

    // Trim leading whitespace
    while (isspace((unsigned char)*str)) str++;

    // If all spaces or empty string
    if (*str == 0) return;

    // Trim trailing whitespace
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
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

        // Default to sequential execution
        cmd.run_if_success = true;

        // Check for `&&`, `||`, or `;`
        if ((sep = strstr(input, "&&")) != NULL) {
            *sep = '\0';
            cmd.run_if_success = true;  // `&&` runs on success
            token = input;
            input = sep + 2;
        } else if ((sep = strstr(input, "||")) != NULL) {
            *sep = '\0';
            cmd.run_if_success = false; // `||` runs on failure
            token = input;
            input = sep + 2;
        } else if ((sep = strchr(input, ';')) != NULL) {
            *sep = '\0';
            cmd.run_if_success = true; // `;` runs unconditionally
            token = input;
            input = sep + 1;
        } else {
            // Last command without any separator
            token = input;
            input += strlen(input);
        }

        // Trim whitespace and set the command
        trim_whitespace(token);
        cmd.command = strdup(token);

        // Store the command in the array
        if (*num_commands >= capacity) {
            capacity *= 2;
            commands = realloc(commands, capacity * sizeof(ParsedCommand));
        }
        commands[(*num_commands)++] = cmd;
    }

    return commands;
}

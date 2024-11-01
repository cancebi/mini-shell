// parser.h
#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

// Structure to hold a parsed command and its condition
typedef struct {
    char *command;
    bool run_if_success;  // `true` if &&, `false` if ||
} ParsedCommand;

// Function to parse the input line into commands based on `;`, `&&`, `||`
ParsedCommand *parse_input(char *input, int *num_commands);

void trim_whitespace(char *str);
#endif // PARSER_H

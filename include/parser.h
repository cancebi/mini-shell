// parser.h
#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

typedef enum {
    COND_SUCCESS,  // Corresponds to &&
    COND_FAILURE,  // Corresponds to ||
    COND_ALWAYS    // Corresponds to ;
} ConditionType;

typedef struct {
    char *command;
    ConditionType condition;
} ParsedCommand;

void remove_quotes(char *str);
ParsedCommand *parse_input(char *input, int *num_commands);
void trim_whitespace(char *str);
#endif

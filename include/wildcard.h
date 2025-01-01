#ifndef WILDCARD_H
#define WILDCARD_H

#include <stdbool.h>

char **expand_wildcard(const char *pattern, int *num_matches);
bool is_escaped(const char *pattern, int index);

#endif
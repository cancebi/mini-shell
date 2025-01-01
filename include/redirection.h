#ifndef REDIRECTION_H
#define REDIRECTION_H

#include "bufferReader.h"

void handle_redirection(char *command);
int handle_pipeline(char *command);

#endif // REDIRECTION_H


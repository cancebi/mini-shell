#include "redirection.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

void setup_redirection(char *fileName, int redirectionType) {
    int fd;
    if (redirectionType == 1) {  // Redirect stdout
        fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("Cannot open output file");
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    } else if (redirectionType == 0) {  // Redirect stdin
        fd = open(fileName, O_RDONLY);
        if (fd == -1) {
            perror("Cannot open input file");
            return;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
}


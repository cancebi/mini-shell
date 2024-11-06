#ifndef MYSH_H
#define MYSH_H

// Function to start the shell loop
void run_shell();
void handle_sigint(int sig);
int change_directory(char *path);
void print_status();
#endif // MYSH_H


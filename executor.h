#ifndef EXECUTOR_H
#define EXECUTOR_H


// Executes a command and returns the exit status
int execute_command(char *command);
void execute_myjobs();
void execute_myfg(int job_id);
void execute_mybg(int job_id);
void substitute_variables(char *command);

#endif // EXECUTOR_H

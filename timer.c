#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS 10

int main() {
    char input[MAX_COMMAND_LENGTH];     // Buffer to store the input
    char command[MAX_COMMAND_LENGTH];   // Buffer to store the command
    char *args[MAX_ARGS];               // Array of pointers to the arguments
    int i, num_args;

    while (1) {
        printf("Enter a command: ");
        fgets(input, sizeof(input), stdin);

        // separate the command and arguments by tokenizing the input
        // The first token in the args array is the command, the others are arguments
        num_args = 0;
        args[num_args++] = strtok(input, " \n");
        while ((args[num_args] = strtok(NULL, " \n")) != NULL) {
            num_args++;
        }

        // Check if the command is "exit"
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        
        // Fork a child process to execute the command
        pid_t pid = fork();

        if (pid == -1) {            // Check if the fork failed
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {      // Child process
            // Child process
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            wait(NULL);
        }
    }

    return 0;
}


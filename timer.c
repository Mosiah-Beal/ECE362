#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS 10

int main() {
    char command[MAX_COMMAND_LENGTH];   // Buffer to store the command
    char *args[MAX_ARGS];               
    int i, num_args;

    while (1) {
        printf("Enter a command: ");
        fgets(command, sizeof(command), stdin);

        // Remove newline character from the command
        command[strcspn(command, "\n")] = '\0';

        // Tokenize the command into arguments
        char *token = strtok(command, " ");
        i = 0;
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;
        num_args = i;

        // Fork a child process to execute the command
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
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
#include <stdbool.h>

// Check if the command is "exit"
if (strcmp(args[0], "exit") == 0) {
    break;
}

// Check if the command is "cd"
if (strcmp(args[0], "cd") == 0) {
    // Change directory to the specified path
    if (num_args > 1) {
        if (chdir(args[1]) != 0) {
            perror("chdir");
        }
    } else {
        printf("Usage: cd <directory>\n");
    }
    continue;
}

// Fork a child process to execute the command
pid_t pid = fork();

if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
} else if (pid == 0) {
    // Child process
    execvp(args[0], args);
    perror("execvp");
    exit(EXIT_FAILURE);
} else {
    // Parent process
    wait(NULL);
}
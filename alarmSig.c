#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <signal.h>
#define TIMEOUT_SECONDS 10


typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);
void alarm_handler(int signum);


int main() {
    char input[256];    // Buffer to store the input
    time_t start_time, current_time;    // Variables to store the start and current time
    
    signal(SIGALRM, alarm_handler);  // Register the signal handler for SIGALRM
    
    printf("Enter a line of text: ");
    time(&start_time);  // Get the current time
    alarm(TIMEOUT_SECONDS); // Set the alarm for 10 seconds


    // Greedy loop, traps the program for 10 seconds unless the user enters something
    while (1) {

        // Check if the user has entered something (until newline is entered or buffer is full)
        if (fgets(input, sizeof(input), stdin) != NULL) {
            
            // User has entered something, so output the line
            printf("Output: %s", input);
            printf("Time taken: %0.2f seconds\n", difftime(current_time, start_time);


            // Clear the alarm
            alarm(0);


            // Update the start time
            //time(&start_time);
            //alarm(TIMEOUT_SECONDS); // Set the alarm for 10 seconds (again)
            break;
        }

        time(&current_time);  // Get the current time

        // Check if the timeout has occurred
        if (current_time - start_time >= TIMEOUT_SECONDS) {
            printf("Timeout occurred. No input received in %e seconds.\n", current_time - start_time);
            break;
        }

        
    }

    return 0;
}


// Signal handler for SIGALRM
void alarm_handler(int signum) {
    printf("Timeout occurred. No input received in %d seconds.\n", TIMEOUT_SECONDS);
    exit(0);
}

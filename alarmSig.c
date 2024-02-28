#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define TIMEOUT_SECONDS 10

int main() {
    char input[256];
    time_t start_time, current_time;

    printf("Enter a line of text: ");

    time(&start_time);  // Get the current time

    while (1) {
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // User has entered something, so output the line
            printf("Output: %s", input);
            break;
        }

        time(&current_time);  // Get the current time

        // Check if the timeout has occurred
        if (current_time - start_time >= TIMEOUT_SECONDS) {
            printf("Timeout occurred. No input received.\n");
            break;
        }

        // Sleep for a short period to avoid busy-waiting
        usleep(100000);  // Sleep for 100 milliseconds
    }

    return 0;
}
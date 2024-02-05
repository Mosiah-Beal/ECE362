#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

int setexit=0;
char *filename = "mywrite.txt";		// File name to write to // Unneeded if using append mode

void sigintHandler(int sig_num)
{
	setexit = 1;
	fflush(stdout);
	exit(0);
}

int main()
{
	FILE *fileptr;

	// Seemingly unused variables, future homework?
	char id[30];
	char name [47];
	char amt[50];

    signal(SIGINT, sigintHandler);

	// Open file for writing and test for any errors.
	fileptr = fopen(filename, "w");		// Write to file (currently overwrites the file if it exists)
	//fileptr = fopen(filename, "a");	// Open the file in append mode (creates the file if it does not exist)
	if (fileptr == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}
	else
	{
		printf("File opened successfully\n");
	}



    while(1) {
		if(setexit) {
			// Close the file if user presses Ctrl-C
			fclose(fileptr);
			printf("File closed successfully\n");
			break;
		}

		printf("Enter string- Cntrl-C to exit \n");

		char *input = malloc(sizeof(char) * 100);	// Allocate memory for the input string, 100 characters seems reasonable
		fgets(input, 100, stdin);	// Get the first 99 characters of input from terminal

		// Parse the input string and append null character at the newline character
		input[strcspn(input, "\n")] = 0;	// Remove the newline character from the input string

		if(setexit == 0){
			// Write to File
			fprintf(fileptr, "%s\n", input);
			printf("File write was successful\n");
		}

		
	}

	// Close the file (if it is not already closed)
	fclose(fileptr);

    return 0;

}

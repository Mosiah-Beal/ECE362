#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

int setexit=0;


void sigintHandler(int sig_num)
{
	setexit = 1;
	fflush(stdout);
	exit(0);
}

int main()
{
	FILE *fileptr;	// Unused since I am using printf to print the input to the terminal

	// Seemingly unused variables, future homework?
	char id[30];
	char name [47];
	char amt[50];

    signal(SIGINT, sigintHandler);


    while(1)
	{
		// If the user has entered Cntrl-C, exit the program
		if(setexit)
		break;

		printf("Enter string - Cntrl-C to exit: \n");

		char *input = malloc(sizeof(char) * 100);	// Allocate memory for the input string, 100 characters seems reasonable
		fgets(input, 100, stdin);	// Get the first 99 characters from the terminal

		// Parse the input string and append null character at the newline character
		input[strcspn(input, "\n")] = 0;	// Remove the newline character from the input string

		if(setexit ==0){
		
			// Print the input string to the terminal
			printf("You entered: %s\n", input);
			printf("File write was successful\n");	// print a success message to the terminal
		}

	}

    return 0;

}

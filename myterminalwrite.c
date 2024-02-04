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
	FILE *fileptr;

	char id[30];
	char name [47];
	char amt[50];

    signal(SIGINT, sigintHandler);


    while(1)
	{
		if(setexit)
		break;

		printf("Enter string - Cntrl-C to exit: \n");

		char *input = malloc(sizeof(char) * 100);	// Allocate memory for the input string
		fgets(input, 100, stdin);	// Get the input from terminal until the user presses Ctrl-C

		// Parse the input string and append null character at the newline character
		input[strcspn(input, "\n")] = 0;	// Remove the newline character from the input string

		if(setexit ==0){
		
			printf("You entered: %s\n", input);
			printf("File write was successful\n");
		}

	}

    return 0;

}

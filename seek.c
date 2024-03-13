#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAX_ROWS     16000
#define MAX_COLS     16000
#define MAX_THREADS     16
#define DETECT_LEN      20


// Make a struct to pass arguments to the batch threads
typedef struct {
    int startRow;   // Start row in the image
    int startCol;   // Start column in the image

    int work;       // The amount of work to be done by the thread
    long threadID;  // The thread ID
} batch_t;

// Default values
int Threads     = 1;
int Rows        = MAX_ROWS;
int Cols        = MAX_COLS;
int Detect_len  = DETECT_LEN;
int Image[MAX_ROWS][MAX_COLS];

// Counter for the number of matches found
int counter = 0;
pthread_mutex_t counter_lock;


void *checkForMatchBatch(void *args);
void matchBatchWork(void);
void makeAnImageDeterministic(void);
int checkArgumentsExplicit(void);

int main(int argc, char *argv[]) {

    // Process command line arguments
    for( argc--, argv++; argc > 0; argc-=2, argv+=2  ) {
        if      (strcmp(argv[0], "-s" ) == 0 ) srand ( atoi(argv[1]) );
        else if (strcmp(argv[0], "-r" ) == 0 ) Rows = atoi(argv[1]);
        else if (strcmp(argv[0], "-c" ) == 0 ) Cols = atoi(argv[1]);
        else if (strcmp(argv[0], "-l" ) == 0 ) Detect_len = atoi(argv[1]);
        else if (strcmp(argv[0], "-t" ) == 0 ) { Threads = atoi(argv[1]);}
        else {   fprintf(stderr, "\nInvalid Arguments\n"); exit(-1); }
    }


    // Check that the arguments are within the correct range
    // Rows and Cols must be less than MAX_ROWS and MAX_COLS
    // Detect_len must be less than or equal to the smaller of Rows and Cols
    // Threads must be 1, 2, 4, 8, or 16
    if( checkArgumentsExplicit() == -1 ) {
        fprintf(stderr, "Invalid parameters chosen\n");
        exit(-1);
    }


    // Valid arguments were passed to the program
    printf("\nRows: %d, Cols: %d, Detect_len: %d, Threads: %d\n", Rows, Cols, Detect_len, Threads);

    // Time the image filling and matching
    struct timespec imageStart, imageEnd, matchStart, matchEnd;

    // Start image filling timer
    printf("Filling the image with random 1s and 0s.\n");

    // Fill the image with random 1s and 0s
    clock_gettime(CLOCK_MONOTONIC, &imageStart);    // Start the timer
    makeAnImageDeterministic();                     // Fill the image randomly (deterministic order for testing purposes)
    clock_gettime(CLOCK_MONOTONIC, &imageEnd);      // End the timer

    // Calculate and print the time difference in milliseconds
    long ImageSeconds = imageEnd.tv_sec - imageStart.tv_sec;
    long ImageMilliseconds = (imageEnd.tv_nsec - imageStart.tv_nsec) / 1000000;
    
    // Fix the time difference if it is negative
    if(ImageMilliseconds < 0) {
        ImageSeconds--;
        ImageMilliseconds += 1000;
    }
    printf("The image has been filled. It took %ld.%03ld seconds.\n", ImageSeconds, ImageMilliseconds);


    // Start checking for matches timer
    printf("Checking for matches.\n");
    
    // Check for matches
    clock_gettime(CLOCK_MONOTONIC, &matchStart);    // Start the timer
    matchBatchWork();                               // Use threads to check for sequences of 1s in the image
    clock_gettime(CLOCK_MONOTONIC, &matchEnd);      // End the timer

    // Calculate and print the time difference in milliseconds
    long MatchSeconds = matchEnd.tv_sec - matchStart.tv_sec;
    long MatchMilliseconds = (matchEnd.tv_nsec - matchStart.tv_nsec) / 1000000;
    
    // Fix the time difference if it is negative
    if(MatchMilliseconds < 0) {
        MatchSeconds--;
        MatchMilliseconds += 1000;
    }
    printf("There were %d matches found. It took %ld.%03ld seconds.\n", counter, MatchSeconds, MatchMilliseconds);

    return 0;
}


/**
 * @brief Check that the arguments passed to the program are valid
 *
 *
 * @note Looks at the global variables Rows, Cols, Detect_len, and Threads
 * which are set by the command line arguments.
 * The arguments are valid if:
 *  - Rows and Cols are less than MAX_ROWS and MAX_COLS
 *  - Detect_len is less than or equal to the smaller of Rows and Cols
 *  - Threads is 1, 2, 4, 8, or 16
 * @return Valid arguments return 0, otherwise -1
 *  
 */
int checkArgumentsExplicit() {
    int minDimension = Rows < Cols ? Rows : Cols;
    
    // Check that the dimensions are within the correct range
    if(Rows > MAX_ROWS) {
        printf("Rows is greater than MAX_ROWS: %d\n", MAX_ROWS);
        return -1;
    }

    if(Cols > MAX_COLS) {
        printf("Cols is greater than MAX_COLS: %d\n", MAX_COLS);
        return -1;
    }

    if(Detect_len > minDimension) {
        printf("Detect_len is greater than the minimum dimension: %d\n", minDimension);
        return -1;
    }
    
    
    // Check that the number of threads is valid
    if( Threads != 1 && Threads != 2 && Threads != 4 && Threads != 8 && Threads != 16 ) {
        printf("Threads is not 1, 2, 4, 8, or 16: %d\n", Threads);
        return -1;
    }

    // Otherwise, the arguments are valid
    return 0;
}

/**
 * @brief makeAnImage - Fill the image with random 1s and 0s
 * 
 * @note Although the image is filled with random 1s and 0s, the order
 * in which the image is filled is deterministic. This is for testing purposes.
 * so that the same image can be used to test the program multiple times.
 * (use a different seed for the random number generator to get a different image)
 * 
 */
void makeAnImageDeterministic() {
    // Fill the image with 1s and 0s
    for(int r=0; r<Rows; r++) {
        for(int c=0; c<Cols; c++) {
            Image[r][c] = rand() % 2;
        }
    }
}

/**
 * @brief matchBatchWork - Divide the work among the threads and check for matches
 * This function will divide the work among the threads and call the function
 * checkForMatchBatch to check for matches. It will then wait for the threads to finish 
 * before returning.
 *
 * @note If the image is small enough (total cells <= 64), it will be printed to the console 
 * @note If the work is less than the number of threads, it just uses the first work threads
 * 
 */
void matchBatchWork() {
    // determine how to split the work among the threads
    int totalWork = Rows * Cols;
    int work = totalWork / Threads;
    int remainder = totalWork % Threads;
    
    // If the work is less than the number of threads, just use the first work threads
    if( totalWork < Threads ) {
        // Give the first couple of threads 1 cell to check, and the others will do nothing
        // If I was willing to rewrite the code, I could split the horizontal and vertical work
        // To eeek out a little more performance
        Threads = totalWork;    
        
        // Each thread will do 1 cell of work
        work = 1;
        remainder = 0;
    }

    // Print the image if it is small enough
    if(totalWork <= 64) {
        // Array is small enough to be printed
        for(int r=0; r<Rows; r++) {
            for(int c=0; c<Cols; c++) {
                printf("%d ", Image[r][c]);
            }
            printf("\n");
        }
    }

    // Inform the user how the work is divided
    //printf("Total work: %d\n", totalWork);
    //printf("Threads: %d\n", Threads);
    //printf("Work per thread: %d\n", work);
    //printf("Remainder: %d\n", remainder);
 

    // Create the array of threads
    pthread_t thread[Threads];

    // Create the array of data to send with the threads
    batch_t threadData[Threads];

    // Create a lock for the results
    pthread_mutex_init(&counter_lock, NULL);


    // Create the batch threads
    int rc;
    for(long t=0; t<Threads; t++){
        // Determine the starting coordinates and the amount of work for each thread
        
        // Use the index to determine the starting coordinates
        int startIndex = t * work;
        int startRow = startIndex / Cols;
        int startCol = startIndex % Cols;        
        
        // Fill in the thread data
        threadData[t].threadID = t;
        threadData[t].startRow = startRow;
        threadData[t].startCol = startCol;

        // The last thread will do the remainder of the work left over
        if( t == Threads-1 ) {
            threadData[t].work = work + remainder;
        }
        else {
            threadData[t].work = work;
        }

        // Create the threads
        rc = pthread_create(&thread[t], NULL, checkForMatchBatch, (void *)&threadData[t]);
        if (rc){
            fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }

    }

    // Wait for the threads to finish
    for(long t=0; t<Threads; t++){
        pthread_join(thread[t], NULL);
    }

    // Destroy the lock
    pthread_mutex_destroy(&counter_lock);

}

/**
 * @brief *checkForMatchBatch
 * 
 * @param args
 * 
 * @note This function will be called by each thread to check for matches
 * Each thread will be given a portion of the image to check
 * This will be dictated by the starting coordinates and the amount of work
 * to be done by each thread.
 * 
 * The thread will check for matches in the image and update the counter
 * if a match is found. It will need to check when it reaches the end of its
 * row and move to the next row. It will return when it reaches the end of
 * its work.
 * 
 */
void *checkForMatchBatch(void *args) {
    // unpack the arguments
    batch_t *batch = (batch_t *)args;
    int startRow = batch->startRow;
    int startCol = batch->startCol;
    int work = batch->work;
    int r, c, length;

    // Procede through the work queue, checking for matches
    for(int i=0; i<work; i++) {
        
        // get the cell coordinates from the index (this is the current cell in the work queue)
        int cell_r = startRow + ((i + startCol) / Cols);    // From the current index, find how many rows to move down
        int cell_c = (startCol + i) % Cols;                 // From the current index, find how many columns to move over

        // See what happened
        // printf("Thread %ld: Image[%d][%d]\n", batch->threadID, cell_r, cell_c);
        // printf("Thread %ld: i: %d, startRow: %d, startCol: %d\n", batch->threadID, i, startRow, startCol);

        r = cell_r;
        c = cell_c;

        // check that there is enough space to achieve a match along the columns
        if( Cols - c >= Detect_len){
            // check for a match along the columns (Detect_len streak of 1s)
            for(length=0; c < Cols; c++){
                // check if the current cell is a 1
                if( Image[r][c] == 1 ) { 
                    // check if streak is long enough
                    if( ++length == Detect_len ) {
                        // match of specified length found

                        // get a lock before updating the counter
                        pthread_mutex_lock(&counter_lock);
                        counter++;
                        pthread_mutex_unlock(&counter_lock);
                        
                        // Match found, no need to continue checking the columns
                        // Done with this cell, move to the next one
                        // printf("Horizontal match found at Image[%d][%d]\n", cell_r, cell_c);
                        break; 
                    }
                }
                else { // Image[row][col] == 0
                    // Streak broken, no need to continue checking the columns
                    break;
                }
            }
        }
        else {
            // No need to continue, not enough space for a match
            // printf("Image[%d][%d]: Not enough space for a match along the columns\n", cell_r, cell_c);
        }

        // Reset the cell coordinates
        r = cell_r;     // From the current index, find how many rows to move down
        c = cell_c;     // From the current index, find how many columns to move over

        // check that there is enough space to achieve a match along the rows
        if( Rows - r >= Detect_len){
            // check for a match along the rows (Detect_len streak of 1s)
            for(length=0; r < Rows; r++) {
                if( Image[r][c] == 1 ) {
                    // check if streak is long enough
                    if( ++length == Detect_len ) { 
                        // match of specified length found

                        // get a lock before updating the counter
                        pthread_mutex_lock(&counter_lock);
                        counter++;
                        pthread_mutex_unlock(&counter_lock);
                        
                        // Match found, no need to continue checking the rows
                        // Done with this cell, move to the next one
                        // printf("Vertical match found at Image[%d][%d]\n", cell_r, cell_c);
                        break; 
                    }
                }
                else { // Image[row][col] == 0
                    // Streak broken, no need to continue checking the rows
                    break;
                }
            }
        }
        else {
            // No need to continue, not enough space for a match
            // printf("Image[%d][%d]: Not enough space for a match along the rows\n", cell_r, cell_c);
        }
    
        // Done with this cell, move to the next one
    }

    // The thread is done with its work
    // printf("Thread %ld done.\n", batch->threadID);
    pthread_exit(NULL);

}


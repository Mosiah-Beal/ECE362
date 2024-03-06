#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define MAX_ROWS     16000
#define MAX_COLS     16000
#define DETECT_LEN      20
#define MAX_THREADS     16


// Make a struct to pass the coordinates to the threads
typedef struct {
    int startRow;   // Start row in the image for the thread
    int endRow;     // End row in the image for the thread

    int startCol;   // Start column in the image for the thread
    int endCol;     // End column in the image for the thread

    long threadID;  // The thread ID
} threadData_t;

typedef struct {
    int row;    // The row of the cell to check
    int col;    // The column of the cell to check
    int first_call;     // The first call to this thread

    long threadID;  // The thread ID
} match_t;



int Threads     = 1;
int Rows        = MAX_ROWS;
int Cols        = MAX_COLS;
int Detect_len  = DETECT_LEN;
int Image[MAX_ROWS][MAX_COLS];
int counter = 0;
pthread_mutex_t counter_lock;


void *checkForMatch(void *args);
void checkMatchWrapper(void);
void makeAnImage(void);
void *makeAnImageThreads(void *threadData_arg);
int checkArguments(void);

int main(int argc, char *argv[]) {
    int found =0;
    int row, col;

    // Process command line arguments
    for( argc--, argv++; argc > 0; argc-=2, argv+=2  ) {
        if      (strcmp(argv[0], "-s" ) == 0 ) srand ( atoi(argv[1]) );
        else if (strcmp(argv[0], "-r" ) == 0 ) Rows = atoi(argv[1]);
        else if (strcmp(argv[0], "-c" ) == 0 ) Cols = atoi(argv[1]);
        else if (strcmp(argv[0], "-l" ) == 0 ) Detect_len = atoi(argv[1]);
        else if (strcmp(argv[0], "-t" ) == 0 ) { Threads = atoi(argv[1]);}
        else {   printf("\nInvalid Arguments\n"); exit(-1); }
    }


    // Check that the arguments are within the correct range
    // Rows and Cols must be less than MAX_ROWS and MAX_COLS
    // Detect_len must be less than or equal to the smaller of Rows and Cols
    // Threads must be 1, 2, 4, 8, or 16
    if( checkArguments() == -1 ) {
        printf("\nInvalid Arguments\n");
        exit(-1);
    }


    // Valid arguments were passed to the program
    printf("\nRows: %d, Cols: %d, Detect_len: %d, Threads: %d\n", Rows, Cols, Detect_len, Threads);

    // FIXME: write checks for when the number of threads is greater than the number of rows/cols

    // Fill the image with random 1s and 0s
    makeAnImage();


    // Check for matches
    checkMatchWrapper();


    // // Check for matches (go to current cell and check right and down for matches of length Detect_len)
    // for(row=0; row < Rows; row++)
    //     for(col=0; col < Cols; col++)
    //         found += checkForMatch(row,col);

    // printf("\nTOTAL DETECTED: %d\n", found);

    exit(0);
}


/**
 * @brief Check that the arguments passed to the program are valid
 *
 *
 * @note The arguments are valid if:
 *  - Rows and Cols are less than MAX_ROWS and MAX_COLS
 *  - Detect_len is less than or equal to the smaller of Rows and Cols
 *  - Threads is 1, 2, 4, 8, or 16
 *  - Valid arguments return 0, otherwise -1
 * 
 * @param rows 
 * @param cols 
 * @param detect_len 
 * @param threads 
 * @return int 
 */
int checkArguments() {
    
    // Find the smallest of rows and cols
    int minDimension = Rows < Cols ? Rows : Cols;
    
    // Check that the dimensions are within the correct range
    if( Rows > MAX_ROWS || Cols > MAX_COLS || Detect_len > minDimension ) {
        return -1;
    }

    // Check that the number of threads is valid
    if( Threads != 1 && Threads != 2 && Threads != 4 && Threads != 8 && Threads != 16 ) {
        return -1;
    }

    // Otherwise, the arguments are valid
    return 0;
}

void checkMatchWrapper() {
    // Create the array of threads
    pthread_t thread[Threads];

    // Create the thread data
    match_t threadData[Threads];

    // determine how to split the work among the threads

    // Create a lock for the results
    pthread_mutex_init(&counter_lock, NULL);


    // Create the threads, send the threads in a circular pattern
    long t;     // thread index (will be incremented and modulo to loop)
    int rc;     // return code from pthread_create
    for(t=0; t<Threads; t++){
        threadData[t].first_call = 1;   // This will be the first call to the thread
    }
    t = 0;      // reset the thread index (should be unnecessary, but just in case)

    // Go through the rows and columns and check for matches
    for(int row=0; row < Rows; row++) {
        for(int col=0; col < Cols; col++) {

            // Check if we need to wait for a thread to finish
            if(threadData[t].first_call == 0) {  
                pthread_join(thread[t], NULL);
            }
            else {
                threadData[t].first_call = 0;   // This is no longer the first call to the thread
            }

            // Pack the arguments for the thread
            threadData[t].row = row;
            threadData[t].col = col;
            threadData[t].threadID = t;

            // Send the thread to check for a match
            rc = pthread_create(&thread[t], NULL, checkForMatch, (void *)&threadData[t]);
            if (rc){
                printf("ERROR; return code from pthread_create() is %d\n", rc);
                exit(-1);
            }

            // Increment the thread index and loop back to 0 if necessary
            t = (t+1) % Threads;
        }
    }


    // Wait for the threads to finish
    printf("Waiting for threads to finish.\n");
    
    // join the threads
    for(t=0; t<Threads; t++){
        pthread_join(thread[t], NULL);
        printf("Thread %ld done.\n", threadData[t].threadID);
    }
    printf("All threads have joined.\n");

    // Destroy the lock
    pthread_mutex_destroy(&counter_lock);
    
    // print the results
    printf("There were %d matches found.\n", counter);
    
}


void *checkForMatch(void *args) {
    // unpack the arguments
    match_t *match = (match_t *)args;
    int row = match->row;
    int col = match->col;
    int r, c, length;
    

    // check that there is enough space to achieve a match along the columns
    if( Cols - col >= Detect_len){
        // check for a match along the columns (Detect_len streak of 1s)
        for(length=0, c=col; c < Cols; c++){
            if( Image[row][c] == 1 ) { 
                // check if streak is long enough
                if( ++length == Detect_len ) {
                    // match of specified length found

                    // get a lock before updating the counter
                    pthread_mutex_lock(&counter_lock);
                    counter++;
                    pthread_mutex_unlock(&counter_lock);
                    
                    // Match found, no need to continue
                    break; 
                }
            }
            else { // Image[row][c] == 0
                // Streak broken, no need to continue
                break;
            }
        }
    }

    // check that there is enough space to achieve a match along the rows
    if( Rows - row >= Detect_len){
        // check for a match along the rows (Detect_len streak of 1s)
        for(length=0, r=row; r < Rows; r++) {
            if( Image[r][col] == 1 ) {
                // check if streak is long enough
                if( ++length == Detect_len ) { 
                    // match of specified length found

                    // get a lock before updating the counter
                    pthread_mutex_lock(&counter_lock);
                    counter++;
                    pthread_mutex_unlock(&counter_lock);
                    
                    // Match found, no need to continue
                    break; 
                }
            }
            else { // Image[r][col] == 0
                // Streak broken, no need to continue
                break;
            }
        }
    }

    // The thread is done
    //printf("Thread %ld done.\n", match->threadID);
}


/**
 * @brief Split the work of creating the image among the threads
 * 
 * @param rows 
 * @param cols 
 * @param threads 
 */
void makeAnImage() {
      // Determine the size of the image and the work for each thread
    // For simplicity, just distribute the work along either the rows
    int work = Rows / Threads;
    int remainder = Rows % Threads;
    long t;

    // Create the array of threads
    pthread_t thread[Threads];

    // Create the thread data
    threadData_t threadData[Threads];

    // Initialize the thread data
    for(t=0; t<Threads; t++) {
        threadData[t].threadID = t;

        // Determine the start and end rows for each thread
        if( t<Threads-1 ) {
            threadData[t].startRow = t * work;
            threadData[t].endRow = threadData[t].startRow + work;
        }
        // The last thread will do the remainder of the work left over
        else {
            threadData[t].startRow = t * work;
            threadData[t].endRow = threadData[t].startRow + work + remainder;
        }

       // For simplicity, each thread will do all the columns
        threadData[t].startCol = 0;
        threadData[t].endCol = Cols;

    }

    // Create the threads
    int rc;
    for(t=0; t<Threads; t++){
        rc = pthread_create(&thread[t], NULL, makeAnImageThreads, (void *)&threadData[t]);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    // Wait for the threads to finish
    for(t=0; t<Threads; t++){
        pthread_join(thread[t], NULL);
    }

    // The image has been created
    printf("The image has been created.\n");
}


/**
 * @brief Fill in the specified rows and columns of the image with random 1s and 0s
 * 
 * @param threadData 
 * @return void* 
 */
void *makeAnImageThreads(void *threadData_arg) {
    
    // Cast the void pointer to a threadData_t pointer
    threadData_t *threadData = (threadData_t *)threadData_arg;
    
    // Indexing variables
    int r, c;
    
    // Loop through the rows for this thread
    for(r=threadData->startRow; r<threadData->endRow; r++) {
        // Loop through the columns for this thread
        for(c=threadData->startCol; c<threadData->endCol; c++) {
            // Fill the image with random 1s and 0s
            Image[r][c] = rand() % 2;
        }
    }
    
    // Exit the thread
    printf("Thread %ld done.\n", threadData->threadID);
    pthread_exit(NULL);
}


/*

    // Thankfully, since we are just reading the image, we don't need to worry
    // about threads fighting over the same memory location. We can just have
    // each thread fill in its own part of the image.

    // Since rows and scanlines are independent, we can have each thread work on their own
    // row/column. The question is how to divide the work among the threads.

    // One method is to give each thread a cell in the image to work on. This is the simplest
    // method, but we could try to split each cell into the vertical and horizontal components

    // Lets start by giving each thread a cell.



    // (2x2 image)
    |1 1|
    |1 1|

    // (4x4 image)
    |1 1 1 1|
    |1 1 1 1|
    |1 1 1 1|
    |1 1 1 1|

    // (8x8 image)                  // len = 2
    |1 1 1 1 1 1 1 1|               |x 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1|               |1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1|               |1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1|               |1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1|               |1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1|               |1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1|               |1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1|               |1 1 1 1 1 1 1 1|

    // (16x16 image)
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    |1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1|
    


*/

int oldCountMatches(int row, int col) {
    int r,c, length;
    int detect = 0;

    // Check for horizontal matches
    for(length=0, c=col; c < Cols; c++){
        if( Image[row][c] == 1 ) { 
            if( ++length == Detect_len ) {
                detect++;
                break; 
            }
        }
        else { 
            break;
        }
    }

   // Check for vertical matches
   for(length=0, r=row; r < Rows; r++) {
      if( Image[r][col] == 1 ) {
        if( ++length == Detect_len ) { 
           detect++; 
           break; 
        }
      }
      else {
        break;
      }
   }

  return detect;
}
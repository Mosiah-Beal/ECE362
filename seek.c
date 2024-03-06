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




int Threads     = 1;
int Rows        = MAX_ROWS;
int Cols        = MAX_COLS;
int Detect_len  = DETECT_LEN;
int Image[MAX_ROWS][MAX_COLS];


int checkForMatch(int row, int col);
void makeAnImage(void);
void *makeAnImageThreads(threadData_t *threadData);
int checkArguments();

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


    // Check for matches (go to current cell and check right and down for matches of length Detect_len)
    for(row=0; row < Rows; row++)
        for(col=0; col < Cols; col++)
            found += checkForMatch(row,col);

    printf("\nTOTAL DETECTED: %d\n", found);

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

void checkMatchWrapper(int row, int col) {
    
}



int checkForMatch(int row, int col) {
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
    pthread_t thread[threads];

    // Create the thread data
    threadData_t threadData[threads];

    // Initialize the thread data
    for(t=0; t<threads; t++) {
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
void *makeAnImageThreads(void *threadData) {
    
    // Cast the void pointer to a threadData_t pointer
    threadData_t *threadData = (threadData_t *)threadData;
    
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


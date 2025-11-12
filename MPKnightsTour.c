#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

// Struct to make it easier to pass location pairs
typedef struct
{
    int r;
    int c;
}location;

pid_t root;
void free_board(int **board, int m);
int length(location *moves);
int within_board(int m, int n, location loc);
int **initialize_board(int m, int n, int r, int c);
void print_board(int **board, int m, int n);
int unvisited_count(int **board, int m, int n);
location *check_valid_moves(int **board, int m, int n, location pos, location start);
int contains(location loc, location *moves);
int dead_end_check(int **board, int m, int n, location *moves);
int closed_tour_check(int **board, int m, int n, location start, location *moves);
int open_tour_check(int **board, int m, int n, location start, location *moves);
// void check_cases(int **board, int m, int n, location start, location *moves, int count, int * pipefd);
void child_process(int **board, location move, location start, int m, int n, int count, int * pipefd);
int process_moves(location *moves, int **board, location start, int count, int m, int n, int * pipefd);

int length( location* moves )
{
    int count = 0;
    for( location* ptr = moves; (*ptr).r!= -100; ptr++ )
    {
        count++;
    }

    return count;
}

// Check if a location is within the board
int within_board( int m, int n, location loc )
{
    if ( ( loc.r >= 0 && loc.r < m) && ( loc.c >= 0 && loc.c < n) )
    {
        return 1;
    }

    return 0;
}

// Free mem allocated to board
void free_board( int ** board, int m)
{
    for( int i = 0; i < m; i++)
    {
        free( *(board + i) );
    }

    free( board );
}
// Board creation
int ** initialize_board( int m, int n, int r, int c)
{
    int ** board = calloc( m, sizeof( int *) );

    for( int i = 0; i < m; i++)
    {
        *(board + i) = calloc( n, sizeof( int ) );
    }
    
    // Marking off the start location as visited
    * ( *(board + r ) + c ) = 1; 

    return board;
}

void print_board(int **board, int m, int n) {
    // Print the top border
    printf("+");
    for (int j = 0; j < n; j++) {
        printf("---+");
    }
    printf("\n");

    // Print the board content with grid lines
    for (int i = 0; i < m; i++) {
        printf("|");
        for (int j = 0; j < n; j++) {
            printf(" %d |", *( *(board + i) + j ) );
        }
        printf("\n");

        // Print the row separator
        printf("+");
        for (int j = 0; j < n; j++) {
            printf("---+");
        }
        printf("\n");
    }
}

// Find the number of unvisited squares in the board
int unvisited_count( int ** board, int m, int n )
{
    int count = 0;
    for( int i = 0; i < m; i++)
    {
        for( int j = 0; j < n; j++ )
        {
            if( *( *( board + i) + j ) == 0 )
            {
                count++;
            }
        }

    }

    return count;
}

// There are 8 possible moves from every location. Check which of them are valid and return
// an array with all valid moves.
location * check_valid_moves( int ** board, int m, int n, location pos, location start ) {
    location * offsets = calloc(8, sizeof(location));
    int valid_move_count = 0;
    *(offsets + 0) = (location){-2, 1};
    *(offsets + 1) = (location){-1, 2};
    *(offsets + 2) = (location){1, 2};
    *(offsets + 3) = (location){2, 1};
    *(offsets + 4) = (location){2, -1};
    *(offsets + 5) = (location){1, -2};
    *(offsets + 6) = (location){-1, -2};
    *(offsets + 7) = (location){-2, -1};

    location * offsetPtr = offsets;

    for (int i = 0; i < 8; i++, offsetPtr++) {
        location move = {pos.r + (*offsetPtr).r, pos.c + (*offsetPtr).c};

        // If the move is within the board and the square is unvisited update the valid move count
        if (within_board(m, n, move) && *(*(board + move.r) + move.c) != 1) 
        {
            valid_move_count++;
        }

        // Check for tours
        // If the move is within the board and all squares have been visited, then we have either an open tour or a closed tour
        // Thus increment the count

        if( within_board( m, n, move) && unvisited_count( board, m, n) == 0 && move.r == start.r && move.c == start.c)
        {
            valid_move_count++;
        }
    }

    location *moves = calloc( valid_move_count+1, sizeof(location) );
    location *movePtr = moves;

    offsetPtr = offsets;

    int counter = 0;

    for (int i = 0; i < 8; i++) {
        location move = {pos.r + (*(offsetPtr+i)).r, pos.c + (*(offsetPtr+i)).c};

        if( within_board( m, n, move) && unvisited_count( board, m, n) == 0 && move.r == start.r && move.c == start.c)
        {
            *( movePtr + counter ) = move;
            counter++;
        }

        else if (within_board(m, n, move) && *(*(board + move.r) + move.c) != 1) {
            *( movePtr + counter ) = move;
            counter++;
        }
        // Check for our tours
        // Since we found that we had a tour earlier, we now handle adding the move to the array
        // If the move we are considering is the same location as start, then we will add it to the array
        // The closed tour check will look for this. Despite the square being '1' (visited) 
        // For open tours, we will be checking for the lack of the start state in the array.
        // The start state will not be added to the array in the case of an open tour becuase the 
        // start state will not fall within the 8 possible moves.
    }
    free( offsets );
    *( moves + valid_move_count ) = (location) {-100, 0};
    return moves;
}

void print_moves(location * moves)
{
    for(location *ptr = moves; (*ptr).r != -100; ptr++)
    {
        printf("(%d,%d),",(*ptr).r,(*ptr).c);
    }
    printf("\n");
    return;
}

int contains( location loc, location * moves )
{
    int l = length( moves );
    for( int i = 0; i < l; i++ )
    {
        if( loc.r == (*(moves + i)).r && loc.c == (*(moves + i)).c)
        {
            return 1;
        }
    }

    return 0;
}

int dead_end_check( int ** board, int m, int n, location * moves )
{
    if( length( moves ) == 0 && unvisited_count( board, m, n ) > 0)
    {
        return 1;
    }

    return 0;
}

int closed_tour_check( int ** board, int m, int n, location start, location * moves )
{
    if( unvisited_count( board, m, n ) == 0 && contains( start, moves ) )
    {
        return 1;
    }

    return 0;
}

int open_tour_check( int ** board, int m, int n, location start, location * moves )
{
    if( unvisited_count( board, m, n ) == 0 && !contains( start, moves ) )
    {
        return 1;
    }

    return 0;
}


int process_moves(location * moves, int ** board, location start, int count, int m, int n, int * pipefd ) {
    if (dead_end_check(board, m, n, moves ) ) 
    {
        free( moves );
        free_board( board, m );
        #ifndef QUIET
            printf( "PID %d: Dead end at move #%d\n", getpid(), count );
        #endif
        close( *(pipefd) );
        close( *(pipefd+1) );
        free( pipefd );
        fflush( stdout );
        exit(count);
    }

    else if ( open_tour_check( board, m, n, start, moves ) ) 
    {
        free( moves );
        free_board( board, m );
        int found = 1;
        #ifndef QUIET
            printf("PID %d: Found an open knight's tour; notifying top-level parent\n", getpid() );
        #endif
        write( *(pipefd+1), &found, sizeof( int ) );
        fflush(stdout);
        close( *(pipefd) );
        close( *(pipefd+1) );
        free( pipefd );
        exit(count);
    }

    else if ( closed_tour_check(board, m, n, start, moves) ) 
    {
        free( moves );
        free_board( board, m );
        int found = 2;
        #ifndef QUIET
            printf("PID %d: Found a closed knight's tour; notifying top-level parent\n", getpid() );
        #endif
        write( *(pipefd+1), &found, sizeof( int ) );
        fflush(stdout);
        close( *(pipefd) );
        close( *(pipefd+1) );
        free( pipefd );
        exit(count);
    }

    else
    {
        fflush(stdout);
    }

    int move_size = length(moves);

    // Handle the case where there is only one move without forking
    if (move_size == 1) 
    {
        location move = *(moves);
        *( *(board + move.r) + move.c ) = 1;
        count++;
        location * new_moves = check_valid_moves(board, m, n, move, start );
        free( moves );
        int result = process_moves(new_moves, board, start, count, m, n, pipefd);
        free ( new_moves );
        free_board( board, m );
        return result;
    }

    // Fork only if there are multiple valid moves
    if (move_size > 1) 
    {
        #ifndef QUIET
            printf( "PID %d: %d possible moves after move #%d; creating %d child processes...\n", getpid(), move_size, count, move_size );
        #endif

        #ifdef NO_PARALLEL
            int max_squares = 0;
        #endif

        for (int i = 0; i < move_size; i++) 
        {
            location move = *(moves + i); 

            pid_t child = fork();

            if (child == -1) 
            {
                fprintf(stderr, "ERROR: fork() failed\n");
            } 

            // CHILD PROCESS
            else if (child == 0) 
            { 
                close( *(pipefd) );
                free( moves );
                *( *(board + move.r) + move.c ) = 1;
                count++;
                location * new_moves = check_valid_moves(board, m, n, move, start );
                int covered = process_moves(new_moves, board, start, count, m, n, pipefd );
                free( new_moves );
                free_board( board, m );
                close( *(pipefd+1) );
                free( pipefd );
                exit( covered );
            } 

            else
            {
                #ifdef NO_PARALLEL
                    int status;
                    waitpid(child, &status, 0);  // **BLOCKING waitpid immediately after fork**
                    
                    if (WIFEXITED(status)) 
                    {
                        // printf("PID %d: EXITING\n", child);
                        int exit_status = WEXITSTATUS(status);
                        if (exit_status > max_squares) 
                        {
                            max_squares = exit_status;
                        }
                    } 
                    else 
                    {
                        printf("PARENT: Child terminated abnormally.\n");
                    }
                #endif
            }
        }
        // Non blocking waitpid
        #ifndef NO_PARALLEL
            int max_squares = 0;
            int remaining_children = move_size;
            while (remaining_children > 0) {
                int status;
                pid_t pid = waitpid(-1, &status, WNOHANG);

                if (pid == 0) 
                {
                    usleep(1000); 
                } 

                else if (pid > 0) 
                {
                    remaining_children--;
                    if (WIFEXITED(status)) 
                    {
                        int exit_status = WEXITSTATUS(status);
                        if (exit_status > max_squares) {
                            max_squares = exit_status;
                        }
                    } 

                    else 
                    {
                        printf("PARENT: Child terminated abnormally.\n");
                    }

                } 

                else 
                {
                    perror("waitpid failed");
                }
            }
        #endif
        free( moves );
        free_board( board, m );
        // Return the max result from all forked processes
        if( root == getpid() )
        {
            return max_squares;
        }
        close( *(pipefd) );
        close( *(pipefd+1) );
        free( pipefd );
        exit( max_squares );
    }
    free_board( board, m );
    free( moves );

    if( root == getpid() )
    {
        return count;
    }
    close( *(pipefd) );
    close( *(pipefd+1) );
    free( pipefd );
    exit( count );
}

int main( int argc, char ** argv )
{
    setvbuf( stdout, NULL, _IONBF, 0 );
    
    root = getpid();

    if( argc < 5 )
    {
        fprintf( stderr, "Invalid number of arguments.\n" );
        return EXIT_FAILURE;
    }

    int m = atoi( *(argv + 1) );
    int n = atoi( *(argv + 2) );
    int r = atoi( *(argv + 3) );
    int c = atoi( *(argv + 4) );

    if( m < 2 || n < 2 || r < 0 || r >= m || c < 0 || c >= n )
    {
        fprintf(stderr, "ERROR: Invalid argument(s)\nUSAGE: ./hw2.out <m> <n> <r> <c>\n" );
        return EXIT_FAILURE;
    }

    int ** board = initialize_board(m, n, r, c);
    int * pipefd = calloc( 2, sizeof( int ) );
    int rc = pipe( pipefd );

    if( rc == -1 )
    {
        perror( "pipe() failed" ); 
        free( pipefd );
        return EXIT_FAILURE;
    }

    printf( "PID %d: Solving the knight's tour problem for a %dx%d board\n", getpid(), m, n );
    printf( "PID %d: Starting at row %d and column %d (move #1)\n", getpid(), r, c );
    fflush( stdout );

    location start = (location){r, c};
    
    // Initial list of valid moves  

    location * moves = check_valid_moves( board, m, n, start, start );
    int coverage = process_moves( moves, board, start, 1, m, n, pipefd );


    int * buffer = calloc( 1, sizeof( int ) );
    int bytesRead;
    int closed = 0;
    int open = 0;
    close( *(pipefd + 1) );
    // Reading information from the pipe
    while ( (bytesRead = read( *(pipefd), buffer, sizeof( int ) ) ) > 0){
        // printf("BUFFER: %d\n", *buffer);
        if( *buffer == 1 )
        {
            open++;
        }   

        else if( *buffer == 2 )
        {
            closed++;
        }   
    }  

    free( buffer );
    close( *(pipefd) );
    free( pipefd );

    if( open > 0 || closed > 0)
    {
        printf( "PID %d: Search complete; found %d open tours and %d closed tours\n", getpid(), open, closed );
    }

    else
    {
        printf( "PID %d: Search complete; best solution(s) visited %d squares out of %d\n", getpid(), coverage, m*n );
    }

    return EXIT_SUCCESS;
}



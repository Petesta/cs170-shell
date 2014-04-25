#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_INPUT_SIZE 1024
#define TRUE 1
#define FALSE 0

// TODO: Input/Output Redirection
// TODO: Whitespace Characters
// TODO: Error Handling (malformed string inputs)

int len(char** array) {
    int length = 0;

    while (1) {
        if (array[length] == NULL) { break; }
        length++;
    }

    return length;
}


// Find the index of a string in an array of strings
// Returns -1 if not found
int findIndex(char* args[], const char* charDelim) {
    int i;

    for (i = 0; i < len(args); i++) {
        if (strcmp(args[i], charDelim) == 0) {
            return i;
        }
    }

    return -1;
}


// Delete a slice out of a char array, returning a new array
// with the elements removed
//
// Ex:
//   array = [a b c d e f]
//   array = deleteSlice(array, 1, 3)
//   // => [a e f]
//
char** deleteSlice(char* array[], int start, int end) {
    int arrayLen = len(array);
    int newCount = arrayLen - (end-start);
    char** newArray = malloc(sizeof(char*) * newCount);

    int i;
    int j = 0;
    for (i=0; i<arrayLen; i++) {
        if (i < start || i > end) {
            *(newArray+j) = strdup(array[i]);
            j++;
        }
    }

    return newArray;
}


char** splitString(char* string, const char charDelim) {
    char delim[2];
    char* tmp        = string;
    char* lastChar   = 0;
    char** result    = 0;
    size_t count     = 0;

    delim[0] = charDelim;
    delim[1] = 0;

    while (*tmp) { // Count how many elements will be extracted
        if (charDelim == *tmp) {
            count++;
            lastChar = tmp;
        }

        tmp++;
    }


    // Add space for trailing token
    // TODO: wtf is this
    count += lastChar < (string + strlen(string) - 1);

    // Add space for null terminator
    count++;

    result = malloc(sizeof(char*) * count);

    if (result) {
        char* token = strtok(string, delim);
        size_t idx  = 0;

        while (token) {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }

        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}


// Handling input and output redirection are basically the same thing -
// grab a FD and set one of our own FDs to it
void handleGenericRedirection(char* args[], int opIdx, int streamFileno, char* mode) {
    char* filename = args[opIdx+1];
    FILE* f = fopen(filename, mode);

    if (f == NULL) {
        printf("ERROR: failed to open redirect file %s\n", filename);
        exit(1);
    }

    if (dup2(fileno(f), streamFileno) < 0) {
        printf("ERROR: in dup2()\n");
        exit(1);
    }
}


// Find specified input file and set our stdin FD to it
// Intended to be run from within child process
// Slicing of operator + filename occurs outside of this function
void handleInputRedirection(char* args[], int inputRedirIdx) {
    handleGenericRedirection(args, inputRedirIdx, STDIN_FILENO, "r");
}


// Find specified output file and set our stdout FD to it
// Intended to be run from within child process
// Slicing of operator + filename occurs outside of this function
void handleOutputRedirection(char* args[], int outputRedirIdx) {
    handleGenericRedirection(args, outputRedirIdx, STDOUT_FILENO, "w+");
}


// Wire up I/O redirection as necessary and fork() / exec()
void handleCommand(char* args[], int argLen, int doWait) {
    int status;
    pid_t pid = fork();

    if (pid < 0) {
        printf("ERROR: fork() failed\n");
        exit(1);
    } else if (pid == 0) {
        // Child

        int inputRedirIdx = findIndex(args, "<");
        if (inputRedirIdx > -1) {
            handleInputRedirection(args, inputRedirIdx);
            args = deleteSlice(args, inputRedirIdx, inputRedirIdx+1);
        }

        int outputRedirIdx = findIndex(args, ">");
        if (outputRedirIdx > -1) {
            handleOutputRedirection(args, outputRedirIdx);
            args = deleteSlice(args, outputRedirIdx, outputRedirIdx+1);
        }


        // Exec
        // --------------------------------------
        if (execvp(args[0], args) < 0) {
            printf("ERROR: execvp() failed\n");
            exit(1);
        }
    } else {
        // Parent

        if (doWait) {
            while (wait(&status) != pid) {
                ; // Wait for child to finish
            }
        }
    }
}


void execute(char* args[]) {
    if (strcmp(args[0], "exit") == 0) { exit(1); }

    // TODO: have to implement cd ?

    int argLen = len(args);

    int doWait;
    if (strcmp(args[argLen-1], "&") != 0) {
        doWait = TRUE; // Normal command
    } else {
        args = deleteSlice(args, argLen-1, argLen-1);
        doWait = FALSE; // bg command
    }

    handleCommand(args, argLen, doWait);
}


int main(int argc, char* argv[]) {
    char inputLine[MAX_INPUT_SIZE];
    char** tokens;

    while (1) {
        if (isatty(STDIN_FILENO)) {
            printf("sish:> ");
        }

        fgets(inputLine, MAX_INPUT_SIZE, stdin);
        strtok(inputLine, "\n"); // Weird way to remove newline from fgets

        tokens = splitString(inputLine, ' ');

        if (strcmp(tokens[0], "\n") == 0) {
            // Empty input, do nothing
            ;
        } else {
            execute(tokens);
        }

        // TODO: exit if not TTY ?
        if (!isatty(STDIN_FILENO)) { exit(1); }
    }

    return 0;
}

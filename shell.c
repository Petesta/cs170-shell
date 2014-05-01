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

// TODO: Pipes
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


// Returns the number of matches to the charDelim, used for counting pipes
int numChar(char* args[], const char* string) {
    int i;
    int count = 0;

    for (i = 0; i < len(args); i++) {
        if (strcmp(args[i], string) == 0) { count++; }
    }

    return count;
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

// ls | wc | grep 1
// i = 5, 3
// ls | wc
// i = 3, 1
char** lastCommand(char* args[]) {
    int j = 0;
    int i = len(args) - 1;

    for (; i > 0; i--) {
        if (strcmp(args[i], "|") == 0) { break; }
    }

    char** newArray = malloc(sizeof(char*) * ((len(args) - 1) - i));
    for (i; i < len(args) - 1; i++) {
        *(newArray + j) = strdup(args[i + 1]);
        j++;
    }

    return newArray;
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
void handleCommand(char* args[], int argLen, int doWait, int pipedCommands) {
    int runs;
    int i = 0;
    int fd[2];
    //int chDir = strcmp(args[0], "cd"); // O confirms `cd` is the first argument

    //if (changeDir == 0) {
    if (strcmp(args[0], "cd") == 0) {
        chdir(args[1]);
    } else {
        pid_t pid;

        if (pipedCommands == 0) {
            int status;
            pid = fork();

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
            } else { // Parent

                if (doWait) {
                    while (wait(&status) != pid) {
                        ; // Wait for child to finish
                    }
                }
            }
        } else { // Executing pipes
            int j, status; 
            pid_t pid2;
            
            char** lastArg = lastCommand(args);
            char*** newArgs = malloc(sizeof(char**) * pipedCommands);

            for (j = 0; j < pipedCommands; j++) {
                newArgs[j] = deleteSlice(args, findIndex(args, "|"), len(args) - 1);
                args = deleteSlice(args, 0, findIndex(args, "|"));
            }

            for (j = 0; j < pipedCommands; j++) {
                if (findIndex(newArgs[j], ">") != -1) {
                    newArgs[j] = deleteSlice(*newArgs, findIndex(newArgs[i], ">"), len(newArgs[j]));
                }
            }

            pid = fork();

            if (pid < 0) {
                printf("ERROR: fork() failed\n");
                exit(1);
            } else if (pid == 0) {

                int inputRedirIdx = findIndex(newArgs[i], "<");
                if (inputRedirIdx > -1) {
                    handleInputRedirection(newArgs[i], inputRedirIdx);
                    newArgs[0] = deleteSlice(*newArgs, inputRedirIdx, inputRedirIdx + 1);
                }

                  while (i < pipedCommands) {
                      i++;
                      pipe(fd);
                      pid2 = fork();

                      if (pid2 == 0) {
                          close(fd[0]);
                          dup2(fd[1], 1);
                          close(fd[1]);


                          if (findIndex(newArgs[i - 1], ">") == -1) {
                              if (execvp(*newArgs[i - 1], newArgs[i - 1]) < 0) {
                                  printf("ERROR: execvp() failed\n");
                                  exit(1);
                              }
                          } else {
                              newArgs[0] = deleteSlice(*newArgs, findIndex(newArgs[i - 1], ">"), len(newArgs[0]));
                              if (execvp(*newArgs[i - 1], newArgs[i - 1]) < 0) {
                                  printf("ERROR: execvp() failed\n");
                                  exit(1);
                              }
                          }

                      } else {
                          if (pid2 < 0) {
                              printf("ERROR: fork() failed\n");
                              exit(1);
                          }

                          close(fd[1]);
                          dup2(fd[0], 0);
                          close(fd[0]);
                      }
                  }

                int outputRedirIdx = findIndex(newArgs[i - 1], ">");
                if (outputRedirIdx > -1) {
                    handleOutputRedirection(args, outputRedirIdx);
                    newArgs[0] = deleteSlice(*newArgs, outputRedirIdx, outputRedirIdx + 1);
                }

                  if (execvp(lastArg[0], lastArg) < 0) {
                      printf("ERROR: execvp() failed\n");
                      exit(1);
                  }
            } else {
                if (pid < 0) {
                    printf("ERROR: fork() failed\n");
                    exit(1);
                }

                if (doWait) { waitpid(pid, &status, 0); }
            }
        }
    }
}


void execute(char* args[]) {
    if (strcmp(args[0], "exit") == 0) { exit(1); }

    int doWait;
    int argLen = len(args);
    int pipeCount = numChar(args, "|");

    if (strcmp(args[argLen-1], "&") != 0) {
        doWait = TRUE; // Normal command
    } else {
        args = deleteSlice(args, argLen-1, argLen-1);
        doWait = FALSE; // Background command
    }

    handleCommand(args, argLen, doWait, pipeCount);
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

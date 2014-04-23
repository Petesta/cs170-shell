#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

const char* ampString  = "&";
const char* emptyString = "";
const char* exitString = "exit";

// Basic commands working (i.e. `ls`, `pwd`)
// Command line arguments working (i.e. `ls -l`)
// Exiting the shell working (i.e. command `exit`)

// TODO: Input/Output Redirection
// TODO: Background Processes
// TODO: Whitespace Characters
// TODO: Error Handling (malformed string inputs)

void exitShell(char *string) { if (strcmp(string, exitString) == 0) { exit(1); }}

int len(char** array) {
    int length = 0;

    while (1) {
        if (array[length] == NULL) { break; }
        length++;
    }

    return length;
}

// TODO: We'll eventually want to drop any string from some specified index
char** dropAmpersand(char** string) {
    int i = 0;
    char delim[2];
    char** newArray;

    delim[0] = ' ';
    delim[1] = 0;
    newArray = (char**)malloc(sizeof(char *) * (len(string) - 1));

    while (i < len(string) - 1) { *(newArray + i++) = strdup(string[i]); }

    return newArray;
}

// void deleteAtPosition(char** array, int pos) {
//     int c;
//     int length = len(array);
//     for (c=pos; c<length-1; c++) {
//          array[c] = array[c+1];
//     }
// }

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

    count += lastChar < (string + strlen(string) - 1); // Add space for trailing token

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */

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

// execpv() can call relative to your position
void execute2(char* array[]) {
    int i = 0;
    int length;
    int status;
    pid_t pid;

    length = len(array);

    if (strcmp(array[length - 1], ampString) != 0 ) { // No background process for this command
        if ((pid = fork()) < 0) { // Failed forked process
            printf("ERROR: failed to fork child process\n");
            exit(1);
        } else if (pid == 0) {  // For the child process
            if (execvp(array[0], array) < 0) { // Execute the command
                printf("ERROR: exec failed\n");
                exit(1);
            }
        } else {                         // For the parent process
            while (wait(&status) != pid) // Wait for completion
                ;
        }
    } else { // TODO: This case handles the background process waiting to happen (i.e. &)

        if ((pid = fork()) == 0) { // Child process
            char** droppedAmp = dropAmpersand(array);

            if (execvp(array[0], droppedAmp) < 0) {
                printf("execvp failed\n");
            } else {
                printf("execvp succeeded\n");
            }
        } else { // Parent process 
            return;
        }
    }
}

// Check if output is being redirected; if so,
// set our stdout to the file descriptor of the specified
// output file
//
// To be run only from within forked child process
void handle_output_redirection(char* args[], int argLen) {
    char* outputFilename = NULL;
    int i;

    for (i = 0; i < argLen; i++) {
        if (strcmp(args[i], ">") == 0) {
            outputFilename = args[i + 1];
            // TODO: hacky way to "slice off" output redirection arguments before
            // passing to execvp()
            args[i] = '\0';
            break;
        }
    }

    if (outputFilename != NULL) {
        FILE* outputFile = fopen(outputFilename, "w+");

        if (outputFile == NULL) {
            printf("ERROR: failed to open output file\n");
            exit(1);
        }

        if (dup2(fileno(outputFile), STDOUT_FILENO) < 0) {
          printf("ERROR: in dup2()\n");
          exit(1);
        }
    }
}


void handle_command(char* args[], int argLen) {
    int status;
    pid_t pid = fork();

    if (pid < 0) {
        printf("ERROR: fork() failed\n");
        exit(1);
    } else if (pid == 0) {
        // Child

        // TODO: handle_input_redirection(args, argLen);
        handle_output_redirection(args, argLen);

        if (execvp(args[0], args) < 0) {
            printf("ERROR: execvp() failed\n");
            exit(1);
        }
    } else {
        // Parent
        while (wait(&status) != pid) {
            ; // Wait for child to finish
        }
    }
}

void handle_bg_command(char** string) {
    //printf("TODO: background process\n");
    pid_t = pid;

    printf("calling handle_bg\n");
    if ((pid = fork()) == 0) {
        char** droppedAmp = dropAmpersand(string);

        printf("droppedAmp = %s\n", *droppedAmp);
        if (execvp(string[0], droppedAmp) < 0) {
            printf("execvp failed\n");
        } else {
            printf("execvp succeeded\n");
        }
    } else { // Parent process
        printf("parent process\n");
        return;
    }
}


void execute(char* args[]) {
    if (strcmp(args[0], "exit") == 0) { exit(1); }

    int argLen = len(args);

    if (strcmp(args[argLen-1], "&") != 0) {
        printf("no background process\n");
        handle_command(args, argLen);
    } else {
        printf("handle_bg_command\n")
        handle_bg_command(args);
    }
}

int main(int argc, char* argv[]) {
    //int i;
    char inputLine[1024];
    char** tokens;

    while (1) {
        /*
        printf("sish:> ");
        gets(inputLine);
        printf("\n");

        if (strcmp(inputLine, emptyString) != 0) {
            tokens = splitString(inputLine, ' ');
            //test = dropAmpersand(tokens);
            exitShell(tokens[0]);
            execute(tokens);
        }
        */
        
        if (isatty(STDIN_FILENO)) {
            printf("sish:> ");
        }

        fgets(inputLine, MAX_INPUT_SIZE, stdin);
        strtok(inputLine, "\n"); // Weird way to remove newline from fgets

        tokens = splitString(inputLine, ' ');
        execute2(tokens);

        // TODO: exit if not TTY ?
        if (!isatty(STDIN_FILENO)) { exit(1); }
    }

    return 0;
}

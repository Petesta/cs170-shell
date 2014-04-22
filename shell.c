#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024

// TODO: Input/Output Redirection
// TODO: Background Processes
// TODO: Whitespace Characters
// TODO: Error Handling (malformed string inputs)

// TODO: dup2/dup(), pipe() and close()



int len(char** array) {
    int length = 0;

    while (1) {
        if (array[length] == NULL) { break; }
        length++;
    }

    return length;
}


// TODO: handle runs of whitespace, ex: "one     two"
char** splitString(char* string, const char charDelim) {
    char delim[2];
    char* tmp      = string;
    char* lastChar = 0;
    char** result  = 0;
    size_t count   = 0;

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

    if (result != NULL) {
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


void handle_command(char* args[]) {
    int status;
    pid_t pid = fork();

    if (pid < 0) {
        printf("ERROR: fork() failed\n");
        exit(1);
    } else if (pid == 0) {
        // Child

        if (execvp(args[0], args) < 0) {
            printf("ERROR: execvp() failed\n");
            exit(1);
        }
    } else {
        // Parent
        while (wait(&status) != pid) {
            // Wait for child to finish
        }
    }
}


void handle_bg_command() {
    printf("TODO: background process\n");
}


void execute(char* args[]) {
    if (strcmp(args[0], "exit") == 0) { exit(1); }

    int argLen = len(args);
    int i;



    // TODO: input redirection




    // Check if output is being redirected; if so,
    // set our stdout to the file descriptor of the specified
    // output file
    char* outputFilename = NULL;
    for (i=0; i<argLen; i++) {
        if (strcmp(args[i], ">") == 0) {
            // TODO: slice > and output file off args so they don't get passed to execvp
            outputFilename = args[i+1];
            break;
        }
    }

    if (outputFilename != NULL) {
        printf("debug: got output file %s\n", outputFilename);

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



    if (strcmp(args[argLen-1], "&") != 0) {
        handle_command(args);
    } else {
        handle_bg_command();
    }
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
        execute(tokens);

        // TODO: exit if not TTY ?
        if (!isatty(STDIN_FILENO)) { exit(1); }
    }

    return 0;
}

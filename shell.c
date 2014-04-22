#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

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


void handle_command(char* args[]) {
    int status;
    pid_t pid = fork();

    if (pid < 0) {
        printf("ERROR: fork() failed\n");
        exit(1);
    } else if (pid == 0) {
        // Child

        // TODO: do we have to do PATH command resolution here?
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
    if (strcmp(args[argLen-1], "&") != 0) {
        handle_command(args);
    } else {
        handle_bg_command();
    }
}



// TODO: where to handle input/output redirection ?


int main(int argc, char* argv[]) {
    char inputLine[1024];
    char** tokens;

    while (1) {
        if (isatty(STDIN_FILENO)) {
            printf("sish:> ");

            // TODO: fgets?
            gets(inputLine);

            tokens = splitString(inputLine, ' ');
            execute(tokens);
        } else {
            printf("TODO: STDIN is not a TTY, input is being redirected\n");
        }
    }

    return 0;
}

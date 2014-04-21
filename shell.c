#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

const char* exitString = "exit";
const char* ampString  = "&";

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
void execute(char* array[]) {
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
        /*
        if ((pid = fork()) == 0) {

        } else {

        }
        */

        printf("You are in the else part\n");
    }
}

int main(int argc, char* argv[]) {
    //int i;
    char inputLine[1024];
    char** tokens;

    while (1) {
        printf("sish:> ");
        gets(inputLine);
        printf("\n");

        tokens = splitString(inputLine, ' ');

        //for (i = 0; *(tokens + i); i++) {
        //    printf("%s\n", tokens[0]);
        //    printf("%s\n", *(tokens + i));
        //    free(*(tokens + i));
        //}

        exitShell(tokens[0]);
        execute(tokens);
        //break; // Only run the shell once if not commented out
    }

    return 0;
}

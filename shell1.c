#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <stdbool.h>
#include <string.h>

bool i, amper, retid, status;
char* argv [10];
char command [1024];
char* token;

void parseCommandLine(char* command, char* argv[]) {
    int i = 0;
    char* token = strtok(command, " ");
    while (token != NULL) {
        argv [i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    argv [i] = NULL;
}

int main() {

    while (1) {
        printf("hello: ");
        fgets(command, 1024, stdin);
        command [strlen(command) - 1] = '\0'; // replace \n with \0

        // print the command line
        printf("command: %s\n", command);

        parseCommandLine(command, argv);

        /* Is command empty */
        if (argv [0] == NULL)
            continue;

        /* Does command line end with & */
        if (!strcmp(argv [i - 1], "&")) {
            amper = true;
            argv [i - 1] = NULL;
        }
        else
            amper = false;

        /* for commands not part of the shell command language */

        if (fork() == 0) {
            execvp(argv [0], argv);
        }
        /* parent continues here */
        if (amper == 0)
            wait(NULL);
    }
}

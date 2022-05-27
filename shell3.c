#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <stdbool.h>
#include <string.h>
#include <signal.h>

char command [1024];
char* token;
int i;
char* outfile;
bool amper, redirect, piping, retid, outerr, concat;
int status, argc1, fd;
int fildes [2];
char* argv1 [10], * argv2 [10];
char* cursor = "hello";
int main();

void parseCommandLine() {
    i = 0;
    token = strtok(command, " ");
    while (token != NULL) {
        argv1 [i++] = token;
        token = strtok(NULL, " ");
        if (token && !strcmp(token, "|")) {
            piping = true;
            break;
        }
    }
    argv1 [i] = NULL;
    argc1 = i;
}

void check_and_handle_piping() {
    if (piping) {
        i = 0;
        while (token != NULL) {
            token = strtok(NULL, " ");
            argv2 [i] = token;
            i++;
        }
        argv2 [i] = NULL;
    }
}

void check_and_handle_redirection() {
    if (argc1 > 1 && !strcmp(argv1 [argc1 - 2], ">")) {
        redirect = true;
        outfile = argv1 [argc1 - 1];
        argv1 [argc1 - 2] = NULL;
        argc1 = argc1 - 2;
    }
    else if (argc1 > 1 && !strcmp(argv1 [argc1 - 2], ">>")) {
        concat = true;
        outfile = argv1 [argc1 - 1];
        argv1 [argc1 - 2] = NULL;
        argc1 = argc1 - 2;
    }
    else
        redirect = false;
}

void check_and_handle_amper() {
    if (argc1 > 0 && !strcmp(argv1 [argc1 - 1], "&")) {
        amper = true;
        argv1 [argc1 - 1] = NULL;
    }
    else
        amper = false;
}

void check_and_handle_stderr() {
    if (argc1 > 1 && !strcmp(argv1 [argc1 - 2], "2>")) {
        outerr = true;
        argv1 [argc1 - 2] = NULL;
        outfile = argv1 [argc1 - 1];
    }
    else
        outerr = false;
}

void check_and_handle_change_cursor() {
    if (argc1 == 3 && !strcmp(argv1 [0], "prompt") && !strcmp(argv1 [1], "=")) {
        cursor = malloc(strlen(argv1 [2]) + 1);
        strcpy(cursor, argv1 [2]);
    }
}

void check_and_handle_echo() {
    if (argc1 > 1 && !strcmp(argv1 [0], "echo")) {
        if (argc1 == 2 && !strcmp(argv1 [1], "$?")) {
            printf("%d\n", status);
        }
        else {
            for (i = 1; i < argc1; i++) {
                if (argv1 [i][0] == '$') {
                    char* var = getenv(argv1 [i] + 1);
                    if (var) {
                        printf("%s ", var);
                    }
                    else {
                        printf("%s ", argv1 [i]);
                        printf("%s ", var);
                    }
                }
                else
                    printf("%s ", argv1 [i]);
            }
            printf("\n");
        }
    }
}

void check_and_handle_cd() {
    if (argc1 > 1 && !strcmp(argv1 [0], "cd")) {
        if (argc1 == 2) {
            if (chdir(argv1 [1]) == -1) {
                printf("%s: No such file or directory\n", argv1 [1]);
            }
        }
        else {
            printf("cd: too many arguments\n");
        }
    }
}

void check_and_handle_quit_shell() {
    if (argc1 == 1 && !strcmp(argv1 [0], "quit")) {
        exit(0);
    }
}

void check_and_handle_variable() {
    if (argc1 == 3 && !strcmp(argv1 [1], "=") && argv1 [0][0] == '$') {
        setenv(argv1 [0] + 1, argv1 [2], 1);
    }
}

void check_and_handle_read() {
    if (argc1 == 2 && !strcmp(argv1 [0], "read")) {
        char* var = argv1 [1];
        char* value = (char*) malloc(sizeof(char) * 1024);
        scanf("%s", value);
        setenv(var, value, 1);
    }
}

void sigintHandler(int sig_num) {
    printf("\nYou typed Control-C! \n");
}

int main() {

    signal(SIGINT, sigintHandler);

    while (true) {
        printf("%s: ", cursor);
        /* Does command line start with !! */
        // check_and_handle_print_last_command();
        fgets(command, 1024, stdin);
        command [strlen(command) - 1] = '\0';
        piping = false;

        /* parse command line */
        parseCommandLine();

        /* Is command empty */
        if (argv1 [0] == NULL)
            continue;

        /* Does command contain pipe */
        check_and_handle_piping();
        /* Does command line end with & */
        check_and_handle_amper();
        /* Does command line end with > */
        check_and_handle_redirection();
        /* Does command line end with 2> */
        check_and_handle_stderr();
        /* Does command line start with prompt */
        check_and_handle_change_cursor();
        /* Does command line start with echo */
        check_and_handle_echo();
        /* Does command line start with cd */
        check_and_handle_cd();
        /* Does command line start with quit */
        check_and_handle_quit_shell();
        /* Does command line is $key = value */
        check_and_handle_variable();
        /* Does command line start with read */
        check_and_handle_read();

        /* for commands not part of the shell command language */
        if (fork() == 0) {
            /* redirection of IO ? */
            if (redirect) {
                fd = creat(outfile, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            if (concat) {
                fd = open(outfile, O_RDWR | O_CREAT | O_APPEND, 0660);
                close(STDOUT_FILENO);
                dup(fd);
                close(fd);
                /* stdout is now redirected */
            }
            if (piping) {
                pipe(fildes);
                if (fork() == 0) {
                    /* first component of command line */
                    close(STDOUT_FILENO);
                    dup(fildes [1]);
                    close(fildes [1]);
                    close(fildes [0]);
                    /* stdout now goes to pipe */
                    /* child process does command */
                    execvp(argv1 [0], argv1);
                }
                /* 2nd command component of command line */
                close(STDIN_FILENO);
                dup(fildes [0]);
                close(fildes [0]);
                close(fildes [1]);
                /* standard input now comes from pipe */
                execvp(argv2 [0], argv2);
            }
            else if (outerr) {
                // redirect to stderr
                fd = creat(outfile, 0660);
                close(STDERR_FILENO);
                dup(fd);
                close(fd);
                /* stderr is now redirected */
                execvp(argv1 [0], argv1);
            }
            else
                execvp(argv1 [0], argv1);
        }
        /* parent continues over here... */
        /* waits for child to exit if required */
        if (amper == false)
            retid = wait(&status);
    }
}

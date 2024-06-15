#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_CHARS 100 // the input can contain at most 100 characters
#define MAX_ARGS 10 // the input can contain at most 10 arguments
#define HISTORY_SIZE 10 // history will print 10 most recently used commands

// 1 - cd <directory> command
int cd_command(char *args[]) {
    if (args[1] == NULL) { // if no arguments present, change it to home directory
        chdir(getenv("HOME"));
        return 0;
    }
    else { 
        if (chdir(args[1]) != 0) { 
            perror("cd failed");
            return 1;
        }
    }
}

// 2 - pwd command
int pwd_command() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("pwd failed");
        return 1;
    }
}

char *history[HISTORY_SIZE]; // a fifo data structure to hold commands in history
int history_count = 0; // intitial count of the commands in history

// 3 - history command
int history_command() {
    for (int i = 0; i < history_count; i++) {
        printf("[%d] %s\n", i + 1, history[i]);
    }
    return 0;
}

// helper method for history command
int add_to_history(const char *command) {
    if (history_count < HISTORY_SIZE) { // if history not full
        history[history_count++] = strdup(command);
    }
    else {
        free(history[0]); // remove the oldest 
        for (int i = 1; i < HISTORY_SIZE; i++) { 
            history[i - 1] = history[i]; // shift all in the history 
        }
        history[HISTORY_SIZE - 1] = strdup(command);
    }
    return 0;
}

// 4 - other commands provided by linux shell 
int other_commands(char *args[], int background) {
    pid_t pid = fork();
    if (pid == 0) { // child process
        if (execvp(args[0], args) == -1) { 
            perror("execvp failed");
        }
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // parent process
        int status;
        if (!background) { // if not background process, wait for child
            wait(&status);
            return WEXITSTATUS(status);
        } else { // if background, display pid
            printf("%d\n", pid);
            return 0;
        }
    } else {
        perror("fork failed");
        return 1;
    }
}

// calls the corresponding methods according to parsed user input
int execute_commands(char *args[], int background) {
    if (strcmp(args[0], "cd") == 0) { return cd_command(args); }
    else if (strcmp(args[0], "pwd") == 0) { return pwd_command(); } 
    else if (strcmp(args[0], "history") == 0) { return history_command(); }
    else if (strcmp(args[0], "exit") == 0) { exit(0); }
    else { return other_commands(args, background); } // support other commands part
}

// parses the piped inputs
void parse_input_for_pipe(char *input) {
    char *left_command[MAX_ARGS], *right_command[MAX_ARGS];
    char *rest = input;
    char *token;
    int i = 0;
    
    // parse the command before pipe operator
    token = strtok(strtok_r(rest, "|", &rest), " \n");
    while(token != NULL && i < MAX_ARGS) {
        left_command[i++] = token;
        token = strtok(NULL, " \n");
    } left_command[i] = NULL; // null-terminate

    // parse the command after pipe operator
    i = 0;
    token = strtok(rest, " \n");
    while (token != NULL && i < MAX_ARGS) {
        right_command[i++] = token;
        token = strtok(NULL, " \n");
    } right_command[i] = NULL; // null-terminate

    int pipefd[2];
    pid_t pid1, pid2;

    if (pipe(pipefd) == -1) {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }

    pid1 = fork();
    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execute_commands(left_command, 0); // assumed no background process can be in pipe
        exit(0);
    }

    pid2 = fork();
    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execute_commands(right_command, 0); // assumed no background process can be in pipe
        exit(0);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

// parse input when logical and operator is used

void parse_input_for_logical_and(char *input) {
    char *first_command[MAX_ARGS], *second_command[MAX_ARGS];
    char *rest = input;
    char *token;
    int i = 0;

    // parsing the command before &&
    token = strtok(strtok_r(rest, "&", &rest), " \n");
    while(token != NULL && i < MAX_ARGS) {
        first_command[i++] = token;
        token = strtok(NULL, " \n");
    }
    first_command[i] = NULL; // null-terminate

    rest++;
    i = 0;

    // parsing the command after &&
    token = strtok(rest, " \n");
    while(token != NULL && i < MAX_ARGS) {
        second_command[i++] = token;
        token = strtok(NULL, " \n");
    }
    second_command[i] = NULL; // null-terminate 

    int status;
    pid_t pid = fork();

    if (pid == 0) { // child
        int ret_val = execute_commands(first_command, 0);
        exit(ret_val);
    } 
    else if (pid > 0){
        wait(&status);
        if (WIFEXITED(status) && (WEXITSTATUS(status) == 0)) {
            execute_commands(second_command, 0);
        }
    }
}

// parses the input taken from user and calls execute methods when no pipe or no AND
void parse_input(char *input) {
    char *args[MAX_ARGS];
    char *token = strtok(input, " \n");
    int background = 0; // 0 for foreground and 1 for background processes
    int i = 0; // counter for the number of arguments

    while (token != NULL && i < MAX_ARGS) { 
        if (strcmp(token, "&") == 0) { // background process
            background = 1;
            break;
        }
        args[i++] = token; // increases i and cont. with next token
        token = strtok(NULL, " \n");
    } args[i] = NULL; // null-terminate

    if (args[0] == NULL) return;

    execute_commands(args, background);
}

// checks if the input contains a pipe (|) or logical and (&&) then executes accordingly
void control(char *input) {
    if(strchr(input, '|') != NULL) { // pipe found
        parse_input_for_pipe(input);
    }
    else if (strstr(input, "&&") != NULL) { // logical AND found
        parse_input_for_logical_and(input);
    }
    else {parse_input(input);} // no pipe or AND found
}

int main() {
    char input[MAX_CHARS];
    
    while (1) {
        printf("myshell> ");
        if (fgets(input, sizeof(input), stdin) == NULL) break; // gets input
        add_to_history(input); // adds the entered input to history
        control(input); // checks if there are pipe or AND, then executes accordingly
    }
    return 0;
}

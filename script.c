#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

// set to 1 to enable debug mode
const int DEBUG_MODE = 1;

void write_to_log_file(char* message) {
    FILE *log_file = fopen("log.txt", "a");
    time_t current_time = time(NULL);
    struct tm *local_time = localtime(&current_time);
    char time_string[100];
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time);
    fprintf(log_file, "[%s] %s\n", time_string, message);
    if (DEBUG_MODE) printf("[%s] %s\n", time_string, message);
    fclose(log_file);
}


void on_child_exit() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Child process %d terminated.\n", pid);
        write_to_log_file("Child terminated");
    }
}

void setup_environment(){
    chdir(getenv("PWD"));
}

void execute_command(char* command) {
    int background = 0;
    char* args[10];
    int i = 0;
    while (command != NULL) {
        args[i++] = command;
        command = strtok(NULL, " ");
        if (command && strcmp(command, "&") == 0) {
            background = 1;
            break;
        }
    }
    args[i] = NULL;

    if (background == 0){
        pid_t pid;
        pid = fork();
        if (pid == 0) {
            // child process
            execvp(args[0], args);
            printf("Error: Command not found.\n");
            exit(1);
        } else if (pid > 0) {
            // parent process
            waitpid(pid, NULL, 0);
        }
    } else {
        pid_t pid;
        pid = fork();
        if (pid == 0) {
            // child process
            execvp(args[0], args);
            printf("Error: Command not found.\n");
            exit(1);
        } else if (pid > 0) {
            // parent process
            printf("Child process %d started.\n", pid);
            write_to_log_file("Child started");
        }
    }
}

void execute_shell_builtin(char* command) {
    // execute shell builtins
    if (strcmp(command, "cd") == 0) {
        command = strtok(NULL, " ");
        if (command == NULL || strcmp(command, "~") == 0){
            command = getenv("HOME");
        }
        chdir(command);
        write_to_log_file("Changed directory");

    } else if (strcmp(command, "echo") == 0) {
        char message[1024] = "";
        // command = strtok(NULL, " ");
        while (command != NULL){
            command = strtok(NULL, " ");
            if (command){
                strcat(message, command);
                strcat(message, " ");
            }
        }
        if (message[0] == '"' && message[strlen(message)-2] == '"'){
            int len = strlen(message);
            if (len >= 2) {
                char result[len-1];
                strncpy(result, message+1, len-2);
                result[len-3] = '\0';
                printf("%s\n", result);
            }
        }
    } else if (strcmp(command, "export") == 0) {
        // export environment variables
        // implementation goes here
    }
}

void shell() {
    char input[1024];
    char* command;
    while (1) {
        // read input
        printf("$ ");
        fgets(input, sizeof(input), stdin);
        input[strlen(input)-1] = '\0';
        command = strtok(input, " ");

        // evaluate input
        if (strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "cd") == 0 || strcmp(command, "echo") == 0 || strcmp(command, "export") == 0) {
            execute_shell_builtin(command);
        } else {
            execute_command(command);
        }
    }
}

int main() {
    write_to_log_file("Shell started");
    signal(SIGCHLD, on_child_exit);
    setup_environment();
    shell();
    write_to_log_file("Shell terminated");
    return 0;
}
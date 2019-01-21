#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INPUT_BUFFER_SIZE 129 // 128 + 1 for null-terminate.
#define MAX_NUM_ARGS 64
#define MAX_NUM_PROCS 20

/////// For Debugging /////////
void print_cmd(char **cmd){
    int i;
    printf("[");
    for (i = 0; i < MAX_NUM_ARGS; i++){
        if (!cmd[i])
            break;
        printf("%s, ", cmd[i]);
    }
    printf("]\n");
}
//////////////////////////////

void print_user_error(){
    fprintf(stderr, "An error has occurred\n");
}

void remove_trailing_newline(char *str){
    str[strcspn(str, "\n")] = 0;
}

void show_prompt(int *history_number){
    printf("mysh (%d)> ", (*history_number)++);
    fflush(stdout);
}

void read_input(char *input_buffer){
    int rc;
    if (fgets(input_buffer, INPUT_BUFFER_SIZE, stdin) == NULL)
        perror("fgets");
    remove_trailing_newline(input_buffer);
}

void parse_input(char *input_buffer, char **cmd){
    char *saveptr, *str, *token;
    int i;
    for (i = 0, str = input_buffer; ; i++, str = NULL){
        token = strtok_r(str, " ", &saveptr);
        if (token == NULL)
            break;
        cmd[i] = token;
    }
}


void get_home_directory(char *buffer){
    if ((buffer = getenv("HOME")) == NULL){
        perror("getenv");
        exit(EXIT_FAILURE);
    }
}
        
int _builtin_cd(char **cmd){
    int rc;
    if (cmd[1] == NULL){
        get_home_directory(cmd[1]);
    }
    rc = chdir(cmd[1]);
    if (rc == -1)
        return 1;
    return 0;
}

int _builtin_pwd(char **cmd){
    char cwd_buf[INPUT_BUFFER_SIZE];
    if (getcwd(cwd_buf, (size_t) INPUT_BUFFER_SIZE) == NULL){
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    printf("%s\n", cwd_buf);
    return 0;
}

int _builtin_exit(char **cmd, pid_t *background_pids){
    int i;
    int sig_check;
    for (i = 0; i < MAX_NUM_PROCS; i++){
        if (!background_pids[i])
            continue;
        sig_check = kill(background_pids[i], 0);
        if (sig_check == 0){
            if (kill(background_pids[i], SIGTERM) == -1){
                perror("kill");
                exit(EXIT_FAILURE);
            }
        }
    }
    exit(0);
    return 0; // Never executes;
}

int execute_command(char **cmd, pid_t *background_procs){
    pid_t child_pid, w;
    int *status;
    if (strcmp("exit", cmd[0]) == 0)
        return _builtin_exit(cmd, background_procs);
    else if (strcmp("cd", cmd[0]) == 0)
        return _builtin_cd(cmd);
    else if (strcmp("pwd", cmd[0]) == 0)
        return _builtin_pwd(cmd);

    child_pid = fork();
    if (child_pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (!child_pid){
        if (execvp(cmd[0], cmd) == -1)
            return 1;
    }
    else {
        w = waitpid(child_pid, status, 0);
        if (w == -1){
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int rc;
    int history_number = 1;
    char input_buffer[INPUT_BUFFER_SIZE];
    char *cmd[MAX_NUM_ARGS];
    pid_t background_procs[MAX_NUM_PROCS];

    while (1) {
        show_prompt(&history_number);
        read_input(input_buffer);
        parse_input(input_buffer, cmd);
        rc = execute_command(cmd, background_procs);
        if (rc == 1)
            print_user_error();
    }

}

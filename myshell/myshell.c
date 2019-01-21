#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INPUT_BUFFER_SIZE 129 // 128 + 1 for null-terminate.
#define MAX_NUM_ARGS 64
#define MAX_NUM_PROCS 20

typedef struct __cmd_t {
    int pipe;
    char **left;
    char **right;
    int redir_output;
    char *output;
    int redir_input;
    char *input;
    int background;
} cmd_t;

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

void tokenize_input(char *input_buffer, char **cmd){
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

int _builtin_exit(pid_t *background_pids){
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

int execute_command(cmd_t *cmd, pid_t *background_procs){
    pid_t child_pid, w, pipe_child_pid;
    int rc;
    int *status;
    int pipefd[2];
    if (strcmp("exit", cmd->left[0]) == 0)
        return _builtin_exit(background_procs);
    else if (strcmp("cd", cmd->left[0]) == 0)
        return _builtin_cd(cmd->left);
    else if (strcmp("pwd", cmd->left[0]) == 0)
        return _builtin_pwd(cmd->left);

    child_pid = fork();
    if (child_pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (child_pid == 0){
        if (cmd->pipe){
            if (pipe(pipefd) == -1){
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            pipe_child_pid = fork();
            if (pipe_child_pid == -1){
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (pipe_child_pid == 0){
                if (close(STDIN_FILENO) == -1){
                    perror("close");
                    exit(EXIT_FAILURE);
                }
                if (close(pipefd[1]) == -1){ // unused write end
                    perror("close");
                    exit(EXIT_FAILURE);
                }
                dup(pipefd[0]);
                if (execvp(cmd->right[0], cmd->right) == -1)
                    print_user_error();
                    /* return 1; */
            }
            else {
                if (close(STDOUT_FILENO) == -1){
                    perror("close");
                    exit(EXIT_FAILURE);
                }
                if (close(pipefd[0]) == -1){ // unused read end
                    perror("close");
                    exit(EXIT_FAILURE);
                }
                dup(pipefd[1]);
                if (execvp(cmd->left[0], cmd->left) == -1)
                    print_user_error();
                    /* return 1; */
            }
        }
        if (cmd->redir_input){
            if (close(STDIN_FILENO) == -1){
                perror("close");
                exit(EXIT_FAILURE);
            }
            if (open(cmd->input, O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR) == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }
        }
        if (cmd->redir_output){
            if (close(STDOUT_FILENO) == -1)
                perror("close");
            if (open(cmd->output, O_CREAT | O_TRUNC | O_WRONLY, S_IWUSR) == -1)
                perror("open");
        }
        if (execvp(cmd->left[0], cmd->left) == -1)
            return 1;
    }
    else if (!cmd->background){
        w = waitpid(child_pid, status, 0);
        if (w == -1){
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        if (cmd->pipe) {
            w = waitpid(pipe_child_pid, status, 0);
            if (w == -1){
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}

void parse_tokens(char **cmd_tokens, cmd_t *cmd){
    int i;
    int right_switch = 0;
    cmd->pipe = 0;
    cmd->redir_output = 0;
    cmd->redir_input = 0;
    cmd->background = 0;
    for (i = 0; i < MAX_NUM_ARGS; i++){
        if (strcmp(cmd_tokens[i], ">") == 0){
            cmd->redir_output = 1;
            cmd->output = cmd_tokens[i+1];
            i++;
            continue;
        }
        if (strcmp(cmd_tokens[i], "<") == 0){
            cmd->redir_input = 1;
            cmd->input = cmd_tokens[i+1];
            i++;
            continue;
        }
        if (strcmp(cmd_tokens[i], "|") == 0){
            cmd->pipe = 1;
            right_switch = i + 1;
            continue;
        }
        if (strcmp(cmd_tokens[i], "&") == 0){
            cmd->background = 1;
            continue;
        }
        if (right_switch == 0)
            cmd->left[i] = cmd_tokens[i];
        else
            cmd->right[i - right_switch] = cmd_tokens[i];
    }
}


int main(int argc, char *argv[]) {
    int rc;
    int history_number = 1;
    char input_buffer[INPUT_BUFFER_SIZE];
    char *cmd_tokens_buf[MAX_NUM_ARGS];
    cmd_t *cmd;
    pid_t background_procs[MAX_NUM_PROCS];

    while (1) {
        show_prompt(&history_number);
        read_input(input_buffer);
        if (!*input_buffer)
            continue;
        tokenize_input(input_buffer, cmd_tokens_buf);
        parse_tokens(cmd_tokens_buf, cmd);
        rc = execute_command(cmd, background_procs);
        if (rc == 1)
            print_user_error();
    }

}

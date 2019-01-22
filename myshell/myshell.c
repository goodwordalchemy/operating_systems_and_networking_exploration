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
    int redir_input;
    int redir_output;
    int background;
    char **left;
    char **right;
    char *output;
    char *input;
} cmd_t;


void init_cmd_t(cmd_t *cmd){
    cmd->pipe = 0;
    cmd->redir_output = 0;
    cmd->redir_input = 0;
    cmd->background = 0;
    cmd->left = malloc(sizeof(char*) * MAX_NUM_ARGS);
    if (cmd->left == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    cmd->right = malloc(sizeof(char*) * MAX_NUM_ARGS);
    if (cmd->right == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
}

void free_cmd_t(cmd_t *cmd){
    free(cmd->right);
    free(cmd->left);
}

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

int tokenize_input(char *input_buffer, char **cmd){
    char *saveptr, *str, *token;
    int i;
    for (i = 0, str = input_buffer; ; i++, str = NULL){
        token = strtok_r(str, " ", &saveptr);
        if (token == NULL)
            break;
        cmd[i] = token;
    }
    return i;
}


void get_home_directory(char **buffer){
    if ((*buffer = getenv("HOME")) == NULL){
        perror("getenv");
        exit(EXIT_FAILURE);
    }
}
        
int _builtin_cd(char **cmd){
    int rc;
    if (cmd[1] == NULL){
        get_home_directory(&cmd[1]);
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

int handle_pipe(cmd_t *cmd){
    int pipefd[2];
    int pipe_child_pid;
    pid_t w;
    int status;
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
            exit(EXIT_FAILURE);
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
            
        w = waitpid(pipe_child_pid, &status, 0);
        if (w == -1){
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        else if (w == 1)
            return 1;
    }
    return 0;
}

void handle_redirect(int old_fd, char *filename){
    if (close(old_fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
    if (open(filename, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU) == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }
}

int track_background_proc(pid_t pid, pid_t *bg_procs){
    int i;
    int sig_check;
    for (i = 0; i < MAX_NUM_PROCS; i++){
        if (bg_procs[i] == 0){
            bg_procs[i] = pid;
            return 0; 
        }
        sig_check = kill(bg_procs[i], 0);
        if (sig_check == -1){
            bg_procs[i] = pid;
            return 0;
        }
    }
    return 1; // out of background procs!
}


////////// FOR DEBUGGING ///////////
void print_command(cmd_t *cmd){
    int i;
    for (i = 0; i < MAX_NUM_ARGS; i++){
        printf("left[%d] = %s\n", i, cmd->left[i]);
    }
}
///////////////////////////////////

int execute_command(cmd_t *cmd, pid_t *background_procs){
    pid_t child_pid, w;
    int rc;
    int status;
    /* print_command(cmd); */
    if (strcmp("exit", cmd->left[0]) == 0)
        return _builtin_exit(background_procs);
    else if (strcmp("cd", cmd->left[0]) == 0)
        return _builtin_cd(cmd->left);
    else if (strcmp("pwd", cmd->left[0]) == 0)
        return _builtin_pwd(cmd->left);

    if ((child_pid = fork()) == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (child_pid == 0){
        if (cmd->pipe){
            rc = handle_pipe(cmd);
            exit(rc);
        }
        if (cmd->redir_input)
            handle_redirect(STDIN_FILENO, cmd->input);
        if (cmd->redir_output)
            handle_redirect(STDOUT_FILENO, cmd->output);
        if (execvp(cmd->left[0], cmd->left) == -1)
            print_user_error();
            exit(1);
    }
    else if (!cmd->background){
        w = waitpid(child_pid, &status, 0);
        if (w == -1){
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        if (w == 1)
            return 1;
    }
    else
        track_background_proc(child_pid, background_procs);
    return 0;
}

void parse_tokens(char **cmd_tokens, cmd_t *cmd, int n_tokens){
    int i;
    int right_switch = 0;
    cmd->pipe = 0;
    cmd->redir_output = 0;
    cmd->redir_input = 0;
    cmd->background = 0;
    for (i = 0; i < n_tokens; i++){
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

void reset_command_state(char *input_buffer, char **cmd_tokens_buf, cmd_t *cmd){
    memset(input_buffer, 0, sizeof(char) * INPUT_BUFFER_SIZE);
    memset(cmd_tokens_buf, 0, sizeof(char*) * MAX_NUM_ARGS);
    memset(cmd->right, 0, sizeof(char*) * MAX_NUM_ARGS);
    memset(cmd->left, 0, sizeof(char*) * MAX_NUM_ARGS);
}

int main(int argc, char *argv[]) {
    int history_number = 1;
    int num_tokens;
    char input_buffer[INPUT_BUFFER_SIZE];
    char *cmd_tokens_buf[MAX_NUM_ARGS];
    cmd_t cmd = { 0 };
    pid_t background_procs[MAX_NUM_PROCS];

    memset(background_procs, 0, sizeof(pid_t) * MAX_NUM_PROCS);
    init_cmd_t(&cmd);
    while (1) {
        show_prompt(&history_number);
        read_input(input_buffer);
        if (!*input_buffer)
            continue;
        num_tokens = tokenize_input(input_buffer, cmd_tokens_buf);
        parse_tokens(cmd_tokens_buf, &cmd, num_tokens);
        execute_command(&cmd, background_procs);
        reset_command_state(input_buffer, cmd_tokens_buf, &cmd);
    }
    free_cmd_t(&cmd);
}


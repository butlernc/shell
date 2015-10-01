/*
 * This code implements a simple shell program
 * It supports the internal shell command "exit", 
 * backgrounding processes with "&", input redirection
 * with "<" and output redirection with ">".
 * However, this is not complete.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

extern char **getaline();

/*
 * Handle exit signals from child processes
 */
void sig_handler(int signal) {
    int status;
    int result = wait(&status);

    printf("Wait returned %d\n", result);
}

/*
 * The main shell function
 */ 
main() {
    int i;
    char **args; 
    int result;
    int block;
    int output;
    int input;
    int append;
    int child_id;
    int status;
    char *output_filename;
    char *input_filename;


    while(1) {

        // Print out the prompt and get the input
        printf("prompt->");
        args = getaline();
        
        // No input, continue
        if(args[0] == NULL)
            continue;

        // Check for internal shell commands, such as exit
        if(internal_command(args))
            continue;

        // Check for an ampersand
        int block = should_block(args);

        if(check_for_pipes(args)) {
            execute_pipe(args, block);
        }

        child_id = do_command(args, 0, 0);
        if(child_id < 0) {
            printf("syntax error\n");
            continue;
        }

        //Wait for the child process to complete, if necessary
        if(block) {
            printf("Waiting for child, pid = %d\n", child_id);
            result = waitpid(child_id, &status, 0);
        }
    }
}

/*
 * Check for ampersand as the last argument
 */
int should_block(char **args) {
    int i;

    for(i = 0; args[i] != NULL; i++) {
        if(args[i][0] == '&') {
            free(args[i]);
            args[i] = NULL;
            return 0;
        } 
    }
    return 1;
}

/* 
 * Check for internal commands
 * Returns true if there is more to do, false otherwise 
 */
int internal_command(char **args) {
    if(strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    return 0;
}

/* 
 * Do the command
 */
int do_command(char **args, int in, int out) {
    char *input_filename, *output_filename;
    // Check for redirected input
    int input = redirect_input(args, &input_filename);

    switch(input) {
    case -1:
        printf("Syntax error!\n");
        return -1;
        break;
    case 0:
        break;
    case 1:
        printf("Redirecting input from: %s\n", input_filename);
        break;
    }

    // check for append
    int append = check_append(args, &output_filename);

    // Check for redirected output
    int output = redirect_output(args, &output_filename);

    switch(append) {
    case -1:
        printf("Syntax error!\n");
        return -1;
        break;
    case 0:
        break;
    case 1:
        printf("Redirecting and appending output to: %s\n", output_filename);
        break;
    }

    switch(output) {
    case -1:
        printf("Syntax error!\n");
        return -1;
        break;
    case 0:
        break;
    case 1:
        printf("Redirecting output to: %s\n", output_filename);
        break;
    }

    pid_t child_id;
    int result;

    // Fork the child process
    child_id = fork();

    // Check for errors in fork()
    switch(child_id) {
    case EAGAIN:
        perror("Error EAGAIN: ");
        return child_id;
    case ENOMEM:
        perror("Error ENOMEM: ");
        return child_id;
    }

    if(child_id == 0) {
        printf("pid ID: %d \n", getpid());
        // // Set up redirection in the child process
        // if(out != 1) { //standard out
        //     dup2(out, 1);
        //     close(out);
        // }
        // if(in != 0) {
        //     dup2(in, 0); 
        //     close(in);
        // }
        if(input)
            freopen(input_filename, "r", stdin);
        if(output)
            freopen(output_filename, "w+", stdout);
        if(append)
            freopen(output_filename, "a", stdout);
        // Execute the command
        printf("append: %d \n", append);
        result = execvp(args[0], args);

        exit(-1);
    }else{
        return child_id;
    }
    return result;
}

int execute_pipe(char **args, int block) {

    int in = 0;
    int child_id = 0;
    char **tmp_args = args;
    char **ptr;
    // loop over args, set each | to NULL
    for(ptr = args; *ptr != NULL; ptr++) {
        //printf("tmpargs[0]: %s [1]: %s [2]: %s\n", tmp_args[0], tmp_args[1], tmp_args[2]);
        if(strcmp(*ptr, "|") == 0) {
            free(*ptr);
            *ptr = NULL;
            // do stuff with tmp_args as your new "args"
            int pipefd[2];
            pipe(pipefd);
            
            child_id = do_command(tmp_args, in, pipefd[1]);
            close(pipefd[1]);
            if(child_id < 0) {
                printf("syntax error\n");
                continue;
            }    
            in = pipefd[0];
            tmp_args = ptr + 1;
        }
    }

    //printf("tmpargs[0]: %s [1]: %s\n", tmp_args[0], tmp_args[1]);
    child_id = do_command(tmp_args, in, 1); 
    if(child_id < 0) {
        printf("syntax error\n");
    }

    return 0;
}

int check_for_pipes(char **args) {
    int i;
    for(i = 0; args[i] != NULL; i++) {
        if(strcmp(args[i], "|") == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * Check for input redirection
 */
int redirect_input(char **args, char **input_filename) {
    int i;
    int j;

    for(i = 0; args[i] != NULL; i++) {

        // Look for the <
        if(args[i][0] == '<') {
            free(args[i]);

        // Read the filename
        if(args[i+1] != NULL) {
            *input_filename = args[i+1];
        } else {
            return -1;
        }

        // Adjust the rest of the arguments in the array
        for(j = i; args[j-1] != NULL; j++) {
            args[j] = args[j+2];
        }

        return 1;
        }
    }

    return 0;
}

/*
 * Check for output redirection: return 1 for regular output, 2 for appending
 */
int redirect_output(char **args, char **output_filename) {
    int i;
    int j;

    for(i = 0; args[i] != NULL; i++) {

        // Look for the >
        if(args[i][0] == '>') {
            free(args[i]);

            // Get the filename 
            if(args[i+1] != NULL) {
                *output_filename = args[i+1];
            } else {
                return -1;
            }

            // Adjust the rest of the arguments in the array
            for(j = i; args[j-1] != NULL; j++) {
                args[j] = args[j+2];
            }   

            return 1;
        }
    }

    return 0;
}


int check_append(char **args, char **output_filename) {
    int i;
    int j;
    for(i = 1; args[i] != NULL; i++) {
        // look for >>
        if(args[i-1][0] == '>' && args[i][0] == '>') {
                
        
            free(args[i-1]);
            free(args[i]);
            
            // get filename
            if(args[i+1] != NULL) {
                *output_filename = args[i+1];
                printf("output: %s \n", *output_filename);
            } else {
                return -1;
            }
            
            // adjust rest of arg array
            for(j = i - 1; args[j-1] != NULL; j++) {
                args[j] = args[j+2];
            }
            printf("args: %s \n", args[1]);
            return 1;
        }
    }
    return 0;
}

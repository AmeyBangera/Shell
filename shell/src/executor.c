#include "executor.h"
#include "input.h"
#include "intrinsics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "utils.h" 
#include "jobs.h"
#include <signal.h>
#include <termios.h>
// ############## LLM Generated Code Begins ##############
// Execute a simple command without redirection/pipes
bool execute_simple_command(char** argv) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return false;
    } else if (pid == 0) {
        // Child process
        execvp(argv[0], argv);
        // If execvp returns, there was an error
        perror("command not found");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}
// Handle input redirection
bool setup_input_redirection(char** argv) {
    // Find the < symbol
    int i;
    for (i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "<") == 0)
            break;
    }
    
    // No input redirection
    if (argv[i] == NULL) return true;
    
    // Make sure there's a file specified
    if (argv[i+1] == NULL) {
        fprintf(stderr, "No input file specified\n");
        return false;
    }
    
    // Open the file
    int fd = open(argv[i+1], O_RDONLY);
    if (fd == -1) {
        perror("No such file or directory");
        return false;
    }
    
    // Redirect stdin to the file
    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2 failed");
        close(fd);
        return false;
    }
    
    close(fd);
    
    // Remove the < and filename from argv
    argv[i] = NULL;  // Terminate the command here
    
    return true;
}
// Handle output redirection
bool setup_output_redirection(char** argv) {
    // Check for > and >> symbols
    int i;
    for (i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], ">>") == 0)
            break;
    }
    
    // No output redirection
    if (argv[i] == NULL) return true;
    
    // Make sure there's a file specified
    if (argv[i+1] == NULL) {
        fprintf(stderr, "No output file specified\n");
        return false;
    }
    
    // Determine if appending
    bool append = (strcmp(argv[i], ">>") == 0);
    
    // Open the file
    int flags = O_WRONLY | O_CREAT;
    if (append) flags |= O_APPEND;
    else flags |= O_TRUNC;
    
    int fd = open(argv[i+1], flags, 0644);
    if (fd == -1) {
        perror("Cannot open output file");
        return false;
    }
    
    // Redirect stdout to the file
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2 failed");
        close(fd);
        return false;
    }
    
    close(fd);
    
    // Remove the > and filename from argv
    argv[i] = NULL;  // Terminate the command here
    return true;
}
// Execute a pipeline of commands
bool execute_pipeline(char* command){

    int pipe_count = 0;
    for (char* p = command; *p; p++) {
        if (*p == '|') pipe_count++;
    }

    if (pipe_count == 0) {
        // No pipes, just a simple command
        char* argv[MAX_INPUT_SIZE / 2];
        int argc = 0;

        char* input_copy = strdup(command);
        split_command(input_copy, argv, &argc);

        if (argc == 0) {
            free(input_copy);
            return true;
        }

        // Check for intrinsics first
        if (is_intrinsic(argv[0])) {
            bool result = execute_intrinsic(argv[0], argc, argv);
            free(input_copy);
            return result;
        }

        // Otherwise fork and exec
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            free(input_copy);
            return false;
        } else if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            
            // Create a new process group if this is first command in pipeline
            if (!setup_input_redirection(argv) || !setup_output_redirection(argv)) {
                exit(EXIT_FAILURE);
            }

            execvp(argv[0], argv);
            perror("command not found");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            free(input_copy);
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }
    }

    // Multiple commands with pipes
    char** commands = malloc((pipe_count + 1) * sizeof(char*));
    if (!commands) {
        perror("malloc failed");
        return false;
    }

    // Split the command by pipes
    char* cmd_copy = strdup(command);
    char* token = strtok(cmd_copy, "|");
    int cmd_count = 0;

    while (token != NULL && cmd_count <= pipe_count) {
        commands[cmd_count++] = token;
        token = strtok(NULL, "|");
    }

    // Create pipes
    int** pipes = malloc(pipe_count * sizeof(int*));
    for (int i = 0; i < pipe_count; i++) {
        pipes[i] = malloc(2 * sizeof(int));
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            // Clean up and exit
            for (int j = 0; j < i; j++) {
                free(pipes[j]);
            }
            free(pipes);
            free(commands);
            free(cmd_copy);
            return false;
        }
    }

    // Execute each command in the pipeline
    pid_t* pids = malloc(cmd_count * sizeof(pid_t));

    for (int i = 0; i < cmd_count; i++) {
        char* argv[MAX_INPUT_SIZE / 2];
        int argc = 0;
        split_command(commands[i], argv, &argc);

        if (argc == 0) continue;

        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork failed");
            continue;
        } else if (pids[i] == 0) {
            // First check if command has input/output redirection
            bool has_input_redir = false, has_output_redir = false;
            for (int k = 0; argv[k] != NULL; k++) {
                if (strcmp(argv[k], "<") == 0) has_input_redir = true;
                if (strcmp(argv[k], ">") == 0 || strcmp(argv[k], ">>") == 0) has_output_redir = true;
            }

            // If no input redirection, and not the first command, use pipe input
            if (!has_input_redir && i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2 failed");
                    exit(EXIT_FAILURE);
                }
            }

            // If no output redirection, and not the last command, use pipe output
            if (!has_output_redir && i < cmd_count - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2 failed");
                    exit(EXIT_FAILURE);
                }
            }

            // Close all pipes (not needed in child anymore)
            for (int j = 0; j < pipe_count; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Now handle explicit redirections if present
            if (!setup_input_redirection(argv) || !setup_output_redirection(argv)) {
                exit(EXIT_FAILURE);
            }

            execvp(argv[0], argv);
            perror("command not found");
            exit(EXIT_FAILURE);
        }
    }

    // Parent closes pipes
    for (int i = 0; i < pipe_count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
        free(pipes[i]);
    }
    free(pipes);

    // Parent waits for all children
    for (int i = 0; i < cmd_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }

    free(pids);
    free(commands);
    free(cmd_copy);

    return true;
}

// Main execution function
bool execute_command(const char* command) {
    char* cmd_copy = strdup(command);
    bool final_result = true;
    
    // Keep track of our position in the string
    char* current_pos = cmd_copy;
    char* cmd_start = current_pos;
    bool in_quotes = false;
    
    // Process the command string character by character
    while (*current_pos) {
        // Handle quotes to prevent splitting inside quoted strings
        if (*current_pos == '"' || *current_pos == '\'') {
            in_quotes = !in_quotes;
            current_pos++;
            continue;
        }
        
        // Only process separators if not in quotes
        if (!in_quotes && (*current_pos == ';' || *current_pos == '&')) {
            // Found a separator - null terminate this command
            char separator = *current_pos;
            *current_pos = '\0';
            
            // Trim trailing whitespace
            char* end = current_pos - 1;
            while (end >= cmd_start && isspace(*end)) {
                *end = '\0';
                end--;
            }
            
            // Execute the command if it's not empty
            if (*cmd_start != '\0') {
                bool background = (separator == '&');
                
                if (background) {
                    pid_t pid = fork();
                    
                    if (pid == -1) {
                        perror("fork failed");
                        final_result = false;
                    } else if (pid == 0) {
                        // Child process
                        
                        // Reset signal handlers to default behavior 
                        signal(SIGINT, SIG_DFL);
                        signal(SIGQUIT, SIG_DFL);
                        signal(SIGTSTP, SIG_DFL);
                        signal(SIGTTIN, SIG_DFL);
                        signal(SIGTTOU, SIG_DFL);
                        signal(SIGCHLD, SIG_DFL);
                        
                        // Create a new process group
                        setpgid(0, 0);
                        
                        // Redirect stdin to /dev/null for background processes
                        int dev_null = open("/dev/null", O_RDONLY);
                        if (dev_null != -1) {
                            dup2(dev_null, STDIN_FILENO);
                            close(dev_null);
                        }
                        
                        // Execute the command
                        execute_pipeline(cmd_start);
                        exit(EXIT_SUCCESS);
                    } else {
                        // Parent process - register background job
                        add_job(pid, cmd_start, true);
                    }
                } else {
                    // Foreground execution
                    pid_t pid = fork();
    
                    if (pid == -1) {
                        perror("fork failed");
                        final_result = false;
                    } else if (pid == 0) {
                        // Child process
                        signal(SIGTSTP, SIG_DFL);
                        signal(SIGINT, SIG_DFL);
                        signal(SIGQUIT, SIG_DFL);
                        signal(SIGTTIN, SIG_DFL);
                        signal(SIGTTOU, SIG_DFL);
                        // Create a new process group
                        setpgid(0, 0);
                        
                        // Execute the command
                        execute_pipeline(cmd_start);
                        exit(EXIT_SUCCESS);
                    } else {
                        // Parent process
                        setpgid(pid, pid);
                        // Set the child as a foreground job
                        int job_id = add_job(pid, cmd_start,false);
                        set_foreground_job(job_id);
                        
                        // Give terminal control to the child process group
                        tcsetpgrp(STDIN_FILENO, getpgid(pid));
                        
                        // Wait for the process to complete or stop
                        int status;
                        waitpid(pid, &status, WUNTRACED);
                        
                        // Take terminal control back
                        tcsetpgrp(STDIN_FILENO, getpgrp());
                        
                        // Check if the process was stopped
                        if (WIFSTOPPED(status)) {
                            // Update the job state to stopped
                            job_t* job = find_job_by_pid(pid);
                            if (job) {
                                job->state = JOB_STOPPED;
                                printf("[%d] Stopped %s\n", job->job_id, job->command);
                                // fprintf(stderr, "Process %d was stopped with status %d\n", 
                                    // (int)pid, WSTOPSIG(status);
                            }
                        } else {
                            // Process completed, clear the job
                            job_t* job = find_job_by_pid(pid);
                            if (job) {
                                job->completed = true;
                                free(job->command);
                                job->job_id = 0;
                                job->pid = 0;
                                job->command = NULL;
                            }
                        }
                        
                        // Clear foreground job
                        clear_foreground_job();
                    }
                    // if (!execute_pipeline(cmd_start)) {
                    //     final_result = false;
                    // }
                }
            }
            
            // Move to the next command
            current_pos++;
            cmd_start = current_pos;
        } else {
            current_pos++;
        }
    }
    
    // Handle the last command (if any)
    if (*cmd_start != '\0') {
        // Trim trailing whitespace
        char* end = current_pos - 1;
        while (end >= cmd_start && isspace(*end)) {
            *end = '\0';
            end--;
        }
        
        // Execute in foreground (no separator at the end)
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork failed");
            final_result = false;
        } else if (pid == 0) {
            // Child process
            signal(SIGTSTP, SIG_DFL);
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            // Create a new process group
            setpgid(0, 0);
            
            // Execute the command
            execute_pipeline(cmd_start);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process
            setpgid(pid, pid);
            // Set the child as a foreground job
            int job_id = add_job(pid, cmd_start,false);
            set_foreground_job(job_id);
            
            // Give terminal control to the child process group
            tcsetpgrp(STDIN_FILENO, getpgid(pid));
            
            // Wait for the process to complete or stop
            int status;
            waitpid(pid, &status, WUNTRACED);
            
            tcsetpgrp(STDIN_FILENO, getpgrp());
            
            if (WIFSTOPPED(status)) {
                // Update the job state to stopped
                job_t* job = find_job_by_pid(pid);
                if (job) {
                    job->state = JOB_STOPPED;
                    printf("[%d] Stopped %s\n", job->job_id, job->command);
                }
            } else {
                // Process completed, clear the job
                job_t* job = find_job_by_pid(pid);
                if (job) {
                    job->completed = true;
                    free(job->command);
                    job->job_id = 0;
                    job->pid = 0;
                    job->command = NULL;
                }
            }
            
            // Clear foreground job
            clear_foreground_job();
        }
    }
    
    free(cmd_copy);
    return final_result;
}
// ############## LLM Generated Code Ends ################
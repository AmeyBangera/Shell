#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include "prompt.h"
#include "input.h"
#include "parser.h"
#include "utils.h"
#include "intrinsics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "executor.h"
#include "jobs.h"
#include <signal.h>
#include <termios.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
// ############## LLM Generated Code Begins ##############
static pid_t shell_pgid;
static struct termios shell_tmodes;
static int shell_terminal;
static int shell_is_interactive;
static sigjmp_buf env_alrm;
static volatile sig_atomic_t jump_active = 0;

void sigchld_handler(int sig) {
    // Save errno in case waitpid changes it
    int saved_errno = errno;
    int status;
    pid_t pid;
    
    // Reap ALL zombie children, not just those we track
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        // Try to find and update job if it exists in our tracking
        job_t* job = find_job_by_pid(pid);
        if (job) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                job->completed = true;
                job->status = status;
            } else if (WIFSTOPPED(status)) {
                job->state = JOB_STOPPED;
            }
        }
        // Note: We reap the process regardless of whether we track it or not
        // This prevents zombie processes from accumulating
    }
    
    // Restore errno
    errno = saved_errno;
}

void sigint_handler(int sig) {
    // Ctrl+C handler
    job_t* fg_job = get_foreground_job();
    if (fg_job) {
        // Send SIGINT to foreground process group
        kill(-fg_job->pgid, SIGINT);
    } else {
        printf("\n");
        fflush(stdout);
        
        if (jump_active) {
            siglongjmp(env_alrm, 1);
        }
    }
}

void sigtstp_handler(int sig) {
    // Ctrl+Z handler
    job_t* fg_job = get_foreground_job();
    if (fg_job) {
        // fprintf(stderr, "Sending SIGTSTP to pgid %d\n", (int)fg_job->pgid);
        kill(-fg_job->pgid, SIGTSTP);
    } else {
        // fprintf(stderr, "No foreground job found for Ctrl+Z\n");
        printf("\n");
        fflush(stdout);
        
        if (jump_active) {
            siglongjmp(env_alrm, 1);
        }
    }
}

void sigterm_handler(int sig) {
    // Clean termination handler
    cleanup_jobs();
    exit(0);
}

// Initialize shell control
void init_shell() {
    // See if we are running interactively
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    
    // Set up termination signal handlers
    sa.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGKILL, &sa, NULL);  // Note: SIGKILL can't be caught, but this doesn't hurt
    
    sa.sa_handler = sigint_handler;
    sigaction(SIGINT, &sa, NULL);
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        // Loop until we are in the foreground
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        // Set up signal handlers with SA_RESTART
        
        sa.sa_handler = sigtstp_handler;
        sigaction(SIGTSTP, &sa, NULL);
        sa.sa_handler = sigchld_handler;
        sigaction(SIGCHLD, &sa, NULL);
        // Ignore other interactive and job-control signals
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);

        // Put ourselves in our own process group
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        // Take control of the terminal
        tcsetpgrp(shell_terminal, shell_pgid);

        // Save default terminal attributes
        tcgetattr(shell_terminal, &shell_tmodes);
    } else {
        // Non-interactive mode - still set up SIGCHLD handler
        sa.sa_handler = sigchld_handler;
        sigaction(SIGCHLD, &sa, NULL);
    }
}
// ############## LLM Generated Code Ends ################
int main() {
    init_shell();
    init_jobs();
    char* home_directory = get_home_directory();
    if (home_directory == NULL) {
        perror("Error getting home directory");
        return EXIT_FAILURE;
    }
    
    set_shell_home(home_directory);
    
    while(1){
        check_jobs();
        if (sigsetjmp(env_alrm, 1) != 0){
            continue;
        }
        
        jump_active = 1;
        display_prompt(home_directory);
        
        char* user_input = get_user_input();
        jump_active = 0;
    
        if(user_input==NULL){
            if (isatty(STDIN_FILENO)) {
                printf("logout\n");
                
                for (int i = 0; i < MAX_JOBS; i++) {
                    job_t* job = &jobs[i];
                    if (job->job_id > 0 && !job->completed) {
                        kill(job->pid, SIGKILL);
                    }
                }
                
                cleanup_jobs();
                free(home_directory);
                exit(0);
            } else {
                // Non-interactive mode: if we can't read input, exit
                cleanup_jobs();
                free(home_directory);
                exit(0);
            }
        }
        
        if (strlen(user_input) > 0) {
            bool valid = parse_input(user_input);
            if (!valid) {
                printf("Invalid Syntax!\n");
            } else {
                if (strncmp(user_input, "log", 3) != 0 || 
                    (user_input[3] != '\0' && !isspace(user_input[3]))) {
                    add_log_entry(user_input);
                }
                
                char* input_copy = strdup(user_input);
                char* argv[MAX_INPUT_SIZE / 2];
                int argc = 0;
                
                split_command(input_copy, argv, &argc);
                
                if (argc > 0 && is_intrinsic(argv[0])) {
                    execute_intrinsic(argv[0], argc, argv);
                } else if (argc > 0) {
                    execute_command(user_input);
                }
                
                free(input_copy);
            }
        }
        
        free(user_input);
    }
    cleanup_jobs();
    free(home_directory);
    
    return EXIT_SUCCESS;
}
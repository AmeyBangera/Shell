#include "jobs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
// ############## LLM Generated Code Begins ##############
job_t jobs[MAX_JOBS];
static int next_job_id = 1;
static int foreground_job = -1;

// Comparison function for sorting jobs by command name
static int compare_jobs_by_command(const void* a, const void* b) {
    job_t* job_a = (job_t*)a;
    job_t* job_b = (job_t*)b;
    
    // Skip empty job slots
    if (job_a->job_id == 0) return 1;
    if (job_b->job_id == 0) return -1;
    
    // Get command names (first word of command)
    char cmd_a[256], cmd_b[256];
    sscanf(job_a->command, "%255s", cmd_a);
    sscanf(job_b->command, "%255s", cmd_b);
    
    return strcmp(cmd_a, cmd_b);
}

void init_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].job_id = 0;
        jobs[i].pid = 0;
        jobs[i].command = NULL;
        jobs[i].completed = false;
        jobs[i].status = 0;
        jobs[i].state = JOB_RUNNING;
        jobs[i].pgid = 0;
    }
    foreground_job = -1;
}

int add_job(pid_t pid, const char* command, bool print_info) {
    // Find an empty slot
    int index = -1;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == 0) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        fprintf(stderr, "Too many background jobs\n");
        return -1;
    }
    
    // Add the job
    jobs[index].job_id = next_job_id++;
    jobs[index].pid = pid;
    jobs[index].command = strdup(command);
    jobs[index].completed = false;
    jobs[index].status = 0;
    jobs[index].state = JOB_RUNNING;
    jobs[index].pgid = getpgid(pid);
    
    // Print job info
    if(print_info) printf("[%d] %d\n", jobs[index].job_id, (int)pid);
    
    return jobs[index].job_id;
}

void check_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id > 0 && !jobs[i].completed) {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
            
            if (result == jobs[i].pid) {
                // Job completed
                jobs[i].completed = true;
                jobs[i].status = status;
                
                // Extract command name (first word)
                char* cmd_copy = strdup(jobs[i].command);
                char* cmd_name = strtok(cmd_copy, " \t\n");
                
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    printf("%s with pid %d exited normally\n", 
                           cmd_name ? cmd_name : jobs[i].command, 
                           (int)jobs[i].pid);
                } else {
                    printf("%s with pid %d exited abnormally\n", 
                           cmd_name ? cmd_name : jobs[i].command, 
                           (int)jobs[i].pid);
                }
                
                free(cmd_copy);
                
                // Clean up the job
                free(jobs[i].command);
                jobs[i].job_id = 0;
                jobs[i].pid = 0;
                jobs[i].command = NULL;
                
                // If this was the foreground job, clear it
                if (foreground_job == i) {
                    foreground_job = -1;
                }
            }
        }
    }
}

void cleanup_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].command) {
            free(jobs[i].command);
            jobs[i].command = NULL;
        }
    }
}

job_t* find_job_by_id(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id) {
            return &jobs[i];
        }
    }
    return NULL;
}

job_t* find_job_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == pid && !jobs[i].completed) {
            return &jobs[i];
        }
    }
    return NULL;
}

void update_job_state(int job_id, job_state_t state) {
    job_t* job = find_job_by_id(job_id);
    if (job) {
        job->state = state;
    }
}

job_t* get_foreground_job() {
    if (foreground_job >= 0 && foreground_job < MAX_JOBS) {
        return &jobs[foreground_job];
    }
    return NULL;
}

void set_foreground_job(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id == job_id) {
            foreground_job = i;
            return;
        }
    }
}

void clear_foreground_job() {
    foreground_job = -1;
}

int get_most_recent_job() {
    int max_job_id = -1;
    int max_index = -1;
    
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id > 0 && !jobs[i].completed && jobs[i].job_id > max_job_id) {
            max_job_id = jobs[i].job_id;
            max_index = i;
        }
    }
    
    if (max_index >= 0) {
        return jobs[max_index].job_id;
    }
    
    return -1;
}

// Activities command - list all running or stopped jobs
bool activities_command() {
    // Create a copy of the jobs array for sorting
    job_t sorted_jobs[MAX_JOBS];
    int job_count = 0;
    
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].job_id > 0 && !jobs[i].completed) {
            // Update job state by checking if process is still alive
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
            
            if (result == 0) {
                // Process still exists - copy to sorted array
                sorted_jobs[job_count++] = jobs[i];
            } else {
                // Process has terminated - mark as completed
                jobs[i].completed = true;
                free(jobs[i].command);
                jobs[i].job_id = 0;
                jobs[i].pid = 0;
                jobs[i].command = NULL;
            }
        }
    }
    
    // Sort jobs by command name
    qsort(sorted_jobs, job_count, sizeof(job_t), compare_jobs_by_command);
    
    // Print sorted jobs
    for (int i = 0; i < job_count; i++) {
        // Get first word of command as command name
        char cmd_name[256];
        sscanf(sorted_jobs[i].command, "%255s", cmd_name);
        
        printf("[%d] : %s - %s\n", 
               (int)sorted_jobs[i].pid, 
               cmd_name, 
               sorted_jobs[i].state == JOB_RUNNING ? "Running" : "Stopped");
    }
    
    return true;
}

// Ping command - send signal to a process
bool ping_command(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: ping <pid> <signal_number>\n");
        return false;
    }
    
    // Parse arguments
    pid_t pid;
    int signal_num;
    
    if (sscanf(argv[1], "%d", &pid) != 1 || 
        sscanf(argv[2], "%d", &signal_num) != 1) {
        fprintf(stderr, "Invalid arguments\n");
        return false;
    }
    
    // Calculate actual signal to send (modulo 32)
    int actual_signal = signal_num % 32;
    if (actual_signal == 0) actual_signal = SIGTERM; // Default to SIGTERM if 0
    
    // Check if process exists by sending signal 0
    if (kill(pid, 0) != 0) {
        fprintf(stderr, "No such process found\n");
        return false;
    }
    
    // Send the signal
    if (kill(pid, actual_signal) == 0) {
        printf("Sent signal %d to process with pid %d\n", actual_signal, pid);
        return true;
    } else {
        perror("Failed to send signal");
        return false;
    }
}

// Foreground command - bring job to foreground
bool fg_command(int argc, char** argv) {
    int job_id;
    
    // Determine which job to bring to foreground
    if (argc > 1) {
        // Parse job ID
        if (sscanf(argv[1], "%d", &job_id) != 1) {
            fprintf(stderr, "Invalid job ID\n");
            return false;
        }
    } else {
        // Use most recent job
        job_id = get_most_recent_job();
        if (job_id < 0) {
            fprintf(stderr, "No jobs available\n");
            return false;
        }
    }
    
    // Find the job
    job_t* job = find_job_by_id(job_id);
    if (!job) {
        fprintf(stderr, "No such job\n");
        return false;
    }
    
    // Print command
    printf("%s\n", job->command);
    
    // Set as foreground job
    set_foreground_job(job_id);
    
    // CRITICAL FIX: Give terminal control to the process group FIRST
    tcsetpgrp(STDIN_FILENO, job->pgid);
    
    // If job was stopped, continue it
    if (job->state == JOB_STOPPED) {
        kill(-job->pgid, SIGCONT);
        job->state = JOB_RUNNING;
    }
    
    // Wait for job to complete or stop again
    int status;
    waitpid(job->pid, &status, WUNTRACED);
    
    // Take terminal control back
    tcsetpgrp(STDIN_FILENO, getpgrp());
    
    // If process was stopped by SIGTSTP
    if (WIFSTOPPED(status)) {
        job->state = JOB_STOPPED;
        printf("[%d] Stopped %s\n", job->job_id, job->command);
    } else {
        // Process completed
        job->completed = true;
        free(job->command);
        job->job_id = 0;
        job->pid = 0;
        job->command = NULL;
    }
    
    // Clear foreground job
    clear_foreground_job();
    
    return true;
}
// Background command - resume stopped job in background
bool bg_command(int argc, char** argv) {
    int job_id;
    
    // Determine which job to resume
    if (argc > 1) {
        // Parse job ID
        if (sscanf(argv[1], "%d", &job_id) != 1) {
            fprintf(stderr, "Invalid job ID\n");
            return false;
        }
    } else {
        // Use most recent job
        job_id = get_most_recent_job();
        if (job_id < 0) {
            fprintf(stderr, "No jobs available\n");
            return false;
        }
    }
    
    // Find the job
    job_t* job = find_job_by_id(job_id);
    if (!job) {
        fprintf(stderr, "No such job\n");
        return false;
    }
    
    // Check if job is already running
    if (job->state == JOB_RUNNING) {
        fprintf(stderr, "Job already running\n");
        return false;
    }
    
    // Send SIGCONT to resume the job
    kill(-job->pgid, SIGCONT);
    
    // Update job state
    job->state = JOB_RUNNING;
    
    // Print job info
    printf("[%d] %s &\n", job->job_id, job->command);
    
    return true;
}
// ############## LLM Generated Code Ends ################
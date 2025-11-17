#ifndef JOBS_H
#define JOBS_H

#include <stdbool.h>
#include <sys/types.h>

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED
} job_state_t;

typedef struct {
    int job_id;
    pid_t pid;
    char* command;
    bool completed;
    int status;
    job_state_t state;
    pid_t pgid;        // Process group ID
} job_t;

#define MAX_JOBS 100

extern job_t jobs[MAX_JOBS];
void init_jobs();

// Add a new background job
int add_job(pid_t pid, const char* command,bool print_info);

// Check for and handle completed background jobs
void check_jobs();

// Clean up jobs system
void cleanup_jobs();

// Find job by job ID
job_t* find_job_by_id(int job_id);

// Find job by process ID
job_t* find_job_by_pid(pid_t pid);

// Update job state
void update_job_state(int job_id, job_state_t state);

// Get current foreground job
job_t* get_foreground_job();

// Set current foreground job
void set_foreground_job(int job_id);

// Clear foreground job
void clear_foreground_job();

// Get most recent job
int get_most_recent_job();

// List all running or stopped processes
bool activities_command();

// Send signal to process
bool ping_command(int argc, char** argv);

// Foreground command implementation
bool fg_command(int argc, char** argv);

// Background command implementation
bool bg_command(int argc, char** argv);

#endif
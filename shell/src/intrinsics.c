#include "intrinsics.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h> 
#include "executor.h"
#define LOG_FILE ".shell_log"
#define MAX_LOG_ENTRIES 15
#define MAX_CMD_LEN 4096

static char prev_dir[PATH_MAX] = "";
static char* home_dir = NULL;

static int compare_strings(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

void set_shell_home(char* dir){
    home_dir = dir;
}

bool is_intrinsic(const char* cmd) {
    return (strcmp(cmd, "hop") == 0 || 
            strcmp(cmd, "reveal") == 0 || 
            strcmp(cmd, "log") == 0 ||
            strcmp(cmd, "activities") == 0 ||
            strcmp(cmd, "ping") == 0 ||
            strcmp(cmd, "fg") == 0 ||
            strcmp(cmd, "bg") == 0);
}

bool execute_intrinsic(const char* cmd, int argc, char** argv) {
    if (strcmp(cmd, "hop") == 0) {
        return hop_command(argc, argv);
    } else if (strcmp(cmd, "reveal") == 0) {
        return reveal_command(argc, argv);
    } else if (strcmp(cmd, "log") == 0) {
        return log_command(argc, argv);
    } else if (strcmp(cmd, "activities") == 0) {
        return activities_command();
    } else if (strcmp(cmd, "ping") == 0) {
        return ping_command(argc, argv);
    } else if (strcmp(cmd, "fg") == 0) {
        return fg_command(argc, argv);
    } else if (strcmp(cmd, "bg") == 0) {
        return bg_command(argc, argv);
    }
    return false;
}

// ############## LLM Generated Code Begins ##############

char* resolve_path(const char* arg) {
    char* target_dir = NULL;
    
    if (arg == NULL || strcmp(arg, "~") == 0) {
        // Home directory
        target_dir = strdup(home_dir);
    } else if (strcmp(arg, ".") == 0) {
        // Current directory
        target_dir = malloc(PATH_MAX);
        if (getcwd(target_dir, PATH_MAX) == NULL) {
            free(target_dir);
            return NULL;
        }
    } else if (strcmp(arg, "..") == 0) {
        // Parent directory
        char current[PATH_MAX];
        if (getcwd(current, PATH_MAX) == NULL) {
            return NULL;
        }
        
        target_dir = strdup(current);
        char* last_slash = strrchr(target_dir, '/');
        if (last_slash != target_dir) {
            *last_slash = '\0';
        } else {
            // Root directory has no parent, stay at root
            target_dir[1] = '\0';
        }
    } else if (strcmp(arg, "-") == 0) {
        // Previous directory
        if (prev_dir[0] == '\0') {
            return NULL; // No previous directory
        }
        target_dir = strdup(prev_dir);
    } else {
        // Normal path
        if (arg[0] == '/') {
            // Absolute path
            target_dir = strdup(arg);
        } else {
            // Relative path
            char current[PATH_MAX];
            if (getcwd(current, PATH_MAX) == NULL) {
                return NULL;
            }
            
            target_dir = malloc(PATH_MAX);
            size_t current_len = strlen(current);
            size_t arg_len = strlen(arg);
            
            // Check if the combined path would fit
            if (current_len + arg_len + 2 > PATH_MAX) {  // +2 for '/' and null terminator
                free(target_dir);
                return NULL;  // Path would be too long
            }
            
            // Safely combine paths
            strcpy(target_dir, current);
            target_dir[current_len] = '/';
            strcpy(target_dir + current_len + 1, arg);
        }
    }
    
    return target_dir;
}

bool hop_command(int argc, char** argv) {
    // No args means go to home directory
    if (argc == 1) {
        return hop_command(2, (char*[]){argv[0], "~", NULL});
    }
    
    // Save current directory for potential use with '-'
    char current[PATH_MAX];
    if (getcwd(current, sizeof(current)) == NULL) {
        perror("getcwd() error");
        return false;
    }
    
    // Process each argument sequentially
    for (int i = 1; i < argc; i++) {
        char* target_dir = resolve_path(argv[i]);
        if (target_dir == NULL) {
            if (strcmp(argv[i], "-") == 0) {
                // Skip if no previous directory
                continue;
            }
            printf("No such directory!\n");
            return false;
        }
        
        // Try to change directory
        if (chdir(target_dir) != 0) {
            printf("No such directory!\n");
            free(target_dir);
            return false;
        }
        
        // If we successfully changed directory, update prev_dir
        // Only update prev_dir if not using '-' command
        if (strcmp(argv[i], "-") != 0) {
            strncpy(prev_dir, current, PATH_MAX);
        }
        
        free(target_dir);
        
        // Update current for next iteration
        if (getcwd(current, sizeof(current)) == NULL) {
            perror("getcwd() error");
            return false;
        }
    }
    
    return true;
}

// reveal command implementation
bool reveal_command(int argc, char** argv) {
    bool show_hidden = false;
    bool line_by_line = false;
    char* path = NULL;
    
    // Parse arguments
    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            // Flag argument
            for (size_t j = 1; j < strlen(argv[i]); j++) {
                if (argv[i][j] == 'a') {
                    show_hidden = true;
                } else if (argv[i][j] == 'l') {
                    line_by_line = true;
                }
            }
        } else {
            // Path argument - only take the first path argument
            if (path == NULL) {
                path = argv[i];
            }
            break;
        }
    }
    
    // Default to current directory if no path specified
    if (path == NULL) {
        path = ".";
    }
    
    char* dir_path = resolve_path(path);
    if (dir_path == NULL) {
        printf("No such directory!\n");
        return false;
    }
    
    // Open directory
    DIR* dir = opendir(dir_path);
    if (dir == NULL) {
        printf("No such directory!\n");
        free(dir_path);
        return false;
    }
    
    // Read directory contents into an array for sorting
    char** filenames = NULL;
    int count = 0;
    int capacity = 10;
    filenames = malloc(capacity * sizeof(char*));
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. entries regardless
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Skip hidden files if not showing hidden
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        // Expand array if needed
        if (count >= capacity) {
            capacity *= 2;
            filenames = realloc(filenames, capacity * sizeof(char*));
        }
        
        filenames[count++] = strdup(entry->d_name);
    }
    
    closedir(dir);
    
    // Sort filenames
    qsort(filenames, count, sizeof(char*), compare_strings);
    
    // Display files
    if (line_by_line) {
        // One file per line (-l option)
        for (int i = 0; i < count; i++) {
            printf("%s\n", filenames[i]);
            free(filenames[i]);
        }
    } else {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        int term_width = w.ws_col;
        
        // Find the longest filename for column width calculation
        size_t max_len = 0;
        for (int i = 0; i < count; i++) {
            size_t len = strlen(filenames[i]);
            if (len > max_len) max_len = len;
        }
        
        // Calculate column width (filename + padding)
        int col_width = max_len + 1;
        if (col_width < 10) col_width = 10; // Minimum width
        
        // Calculate number of columns that fit
        int cols = term_width / col_width;
        if (cols == 0) cols = 1;
        
        // Calculate number of rows needed
        int rows = (count + cols - 1) / cols;
        
        // Print in column-major order
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                int idx = col * rows + row;
                if (idx < count) {
                    printf("%-*s", col_width, filenames[idx]);
                    free(filenames[idx]);
                }
            }
            printf("\n");
        }
    }
    
    // Clean up
    free(filenames);
    free(dir_path);
    
    return true;
}

// Helper functions for log command
static char** read_log_entries(int* count) {
    char** entries = malloc(MAX_LOG_ENTRIES * sizeof(char*));
    *count = 0;
    
    // Create path to log file
    char log_path[PATH_MAX];
    snprintf(log_path, PATH_MAX, "%s/%s", home_dir, LOG_FILE);
    
    // Try to open log file
    FILE* log_file = fopen(log_path, "r");
    if (log_file == NULL) {
        return entries;
    }
    
    // Read entries
    char line[MAX_CMD_LEN];
    while (*count < MAX_LOG_ENTRIES && fgets(line, sizeof(line), log_file)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        entries[*count] = strdup(line);
        (*count)++;
    }
    
    fclose(log_file);
    return entries;
}

static void write_log_entries(char** entries, int count) {
    // Create path to log file
    char log_path[PATH_MAX];
    snprintf(log_path, PATH_MAX, "%s/%s", home_dir, LOG_FILE);
    
    // Open log file
    FILE* log_file = fopen(log_path, "w");
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }
    
    // Write entries
    for (int i = 0; i < count; i++) {
        fprintf(log_file, "%s\n", entries[i]);
    }
    
    fclose(log_file);
}

// Add entry to log
void add_log_entry(const char* cmd) {
    // Don't log 'log' commands
    if (strncmp(cmd, "log", 3) == 0 && (cmd[3] == '\0' || isspace(cmd[3]))) {
        return;
    }
    
    int count;
    char** entries = read_log_entries(&count);
    
    // Check if command is the same as last entry
    if (count > 0 && strcmp(entries[count - 1], cmd) == 0) {
        // Free memory and return - don't add duplicate
        for (int i = 0; i < count; i++) {
            free(entries[i]);
        }
        free(entries);
        return;
    }
    
    // Add new entry
    if (count == MAX_LOG_ENTRIES) {
        // Remove oldest entry
        free(entries[0]);
        for (int i = 0; i < count - 1; i++) {
            entries[i] = entries[i + 1];
        }
        count--;
    }
    
    entries[count++] = strdup(cmd);
    
    // Write back to file
    write_log_entries(entries, count);
    
    // Clean up
    for (int i = 0; i < count; i++) {
        free(entries[i]);
    }
    free(entries);
}

// log command implementation
bool log_command(int argc, char** argv) {
    // No arguments - print log
    if (argc == 1) {
        int count;
        char** entries = read_log_entries(&count);
        
        for (int i = 0; i < count; i++) {
            printf("%s\n", entries[i]);
            free(entries[i]);
        }
        free(entries);
        return true;
    }
    
    // purge - clear log
    if (argc == 2 && strcmp(argv[1], "purge") == 0) {
        char log_path[PATH_MAX];
        snprintf(log_path, PATH_MAX, "%s/%s", home_dir, LOG_FILE);
        
        FILE* log_file = fopen(log_path, "w");
        if (log_file != NULL) {
            fclose(log_file);
        }
        return true;
    }
    
    // execute <index> - execute command at index
    if (argc == 3 && strcmp(argv[1], "execute") == 0) {
        int index = atoi(argv[2]);
        if (index < 0) {
            printf("Invalid index\n");
            return false;
        }
        
        int count;
        char** entries = read_log_entries(&count);
        
        int array_index = count - 1 - index;
        
        if (array_index < 0 || array_index >= count) {
            printf("Invalid index\n");
            for (int i = 0; i < count; i++) {
                free(entries[i]);
            }
            free(entries);
            return false;
        }
        
        // Actually execute the command
        char* cmd_to_execute = strdup(entries[array_index]);
        // printf("Executing: %s\n", cmd_to_execute);
        
        // Clean up log entries
        for (int i = 0; i < count; i++) {
            free(entries[i]);
        }
        free(entries);
        
        // Parse and execute the command
        char* cmd_argv[MAX_INPUT_SIZE / 2];
        int cmd_argc = 0;
        
        char* cmd_copy = strdup(cmd_to_execute);
        split_command(cmd_copy, cmd_argv, &cmd_argc);
        
        bool result = false;
        if (cmd_argc > 0) {
            if (is_intrinsic(cmd_argv[0])) {
                result = execute_intrinsic(cmd_argv[0], cmd_argc, cmd_argv);
            } else {
                result = execute_command(cmd_to_execute);
            }
        }
        
        free(cmd_copy);
        free(cmd_to_execute);
        return result;
    }
    
    printf("Invalid log command\n");
    return false;
}
// ############## LLM Generated Code Ends ################
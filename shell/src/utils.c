#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
// ############## LLM Generated Code Begins ##############

void split_command(char* input, char** argv, int* argc) {
    *argc = 0;
    char* token = strtok(input, " \t\n");
    
    while (token != NULL && *argc < MAX_INPUT_SIZE / 2) {
        argv[(*argc)++] = token;
        token = strtok(NULL, " \t\n");
    }
    
    argv[*argc] = NULL;
}
// ############## LLM Generated Code Ends ################
char* get_home_directory(){
    char* home_dir = (char*)malloc(PATH_MAX);
    if(home_dir == NULL){
        return NULL;
    }
    
    if(getcwd(home_dir, PATH_MAX) ==NULL){
        free(home_dir);
        return NULL;
    }
    return home_dir;
}

bool is_subdirectory(const char* path, const char* potential_parent){
    size_t parent_len =strlen(potential_parent);
    
    if(strlen(path) < parent_len){
        return false;
    }
    
    if(strncmp(path, potential_parent, parent_len) == 0){
        return (path[parent_len] == '\0' || path[parent_len] == '/');
    }
    
    return false;
}
// ############## LLM Generated Code Begins ##############
char* format_path(const char* current_path, const char* home_path) {
    char* formatted_path = (char*)malloc(PATH_MAX);
    if (formatted_path == NULL) {
        return NULL;
    }
    
    if(is_subdirectory(current_path, home_path)) {
        if (strcmp(current_path, home_path) == 0) {
            strcpy(formatted_path, "~");
        } else {
            sprintf(formatted_path, "~%s", current_path + strlen(home_path));
        }
    } else {
        // Use absolute path
        strcpy(formatted_path, current_path);
    }
    
    return formatted_path;
}
// ############## LLM Generated Code Ends ################
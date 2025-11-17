#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

char* get_user_input(){
    char* input = (char*)malloc(MAX_INPUT_SIZE);
    if(input == NULL){
        perror("malloc failed");
        return NULL;
    }
    
    // Clear any previous content
    memset(input, 0, MAX_INPUT_SIZE);
    
    // Use fgets to read input
    char* result = fgets(input, MAX_INPUT_SIZE, stdin);
    
    if(result == NULL){
        // fgets returned NULL - could be EOF or error
        if (feof(stdin)) {
            // EOF detected (Ctrl+D was pressed or stdin closed)
            free(input);
            return NULL;
        } else if (ferror(stdin)) {
            // Error occurred - clear it and try to continue
            clearerr(stdin);
            free(input);
            // Return empty string to continue the shell loop
            return strdup("");
        } else {
            // Shouldn't happen, but handle gracefully
            free(input);
            return strdup("");
        }
    }
    
    // Successfully read input - remove trailing newline if present
    size_t len = strlen(input);
    if(len > 0 && input[len-1] == '\n'){
        input[len-1] = '\0';
    }
    
    return input;
}
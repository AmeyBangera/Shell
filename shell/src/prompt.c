#include "prompt.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <sys/types.h>
// ############## LLM Generated Code Begins ##############
void display_prompt(const char* home_path) {
    struct passwd *pw = getpwuid(getuid());
    const char *username = pw ? pw->pw_name : "unknown";
    
    char hostname[HOST_NAME_MAX + 1];
    gethostname(hostname, HOST_NAME_MAX);
    hostname[HOST_NAME_MAX] = '\0';
    
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return;
    }
    
    char* formatted_path = format_path(cwd, home_path);
    if (formatted_path == NULL) {
        perror("Error formatting path");
        return;
    }
    // ############## LLM Generated Code Ends ################
    printf("<%s@%s:%s> ", username, hostname, formatted_path);
    fflush(stdout);
    free(formatted_path);
}
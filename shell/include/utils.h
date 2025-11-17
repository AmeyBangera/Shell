#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
void split_command(char* input, char** argv, int* argc);
char* get_home_directory();
bool is_subdirectory(const char* path, const char* potential_parent);
char* format_path(const char* current_path, const char* home_path);
#define MAX_INPUT_SIZE 4096
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef isspace
#include <ctype.h>
#endif

#endif
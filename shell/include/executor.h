// executor.h
#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdbool.h>

bool execute_command(const char* command);
bool execute_pipeline(char* command);

#endif
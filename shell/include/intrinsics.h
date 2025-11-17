#ifndef INTRINSICS_H
#define INTRINSICS_H

#include <stdbool.h>

bool hop_command(int argc, char** argv);
void set_shell_home(char* dir);
void add_log_entry(const char* cmd);
bool reveal_command(int argc, char** argv);
bool log_command(int argc, char** argv);
bool is_intrinsic(const char* cmd);
bool execute_intrinsic(const char* cmd, int argc, char** argv);
bool activities_command();
bool ping_command(int argc, char** argv);
bool fg_command(int argc, char** argv);
bool bg_command(int argc, char** argv);
#endif
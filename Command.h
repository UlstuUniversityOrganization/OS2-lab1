#ifndef COMMAND_H
#define COMMAND_H
#include "util.h"

typedef struct {
    int tokens_count;
    char** tokens;
} Command;

Command* init_command();

Command* init_command_from_str(char* str);

#endif
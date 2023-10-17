#ifndef COMMAND_H
#define COMMAND_H
#include "util.h"

typedef struct {
    int tokens_count;
    char** tokens;
    // char* output_stream_path;
} Command;

Command* init_command();

Command* init_command_from_str(char* str);

#endif
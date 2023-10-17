#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// typedef struct {
//     int tokens_count;
//     char** tokens;
//     char* output_stream_path;
// } Command;

// разбиение строки на токены
void tokenize(char* input, char** tokens, int* token_count);

// Command* init_command();

// Command* init_command_from_str(char* str);

// компаратор для имен файлов и папок
int compare_filenames(const void* a, const void* b);

void throw_error(const char* message);

char* strjoin(char* str1, const char* str2);

#endif

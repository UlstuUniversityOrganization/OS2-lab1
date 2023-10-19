#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// разбиение строки на токены
void tokenize(char* input, char*** tokens, int* token_count);

// компаратор для имен файлов и папок
int compare_filenames(const void* a, const void* b);

void throw_error(const char* message);

char* strjoin(char* str1, const char* str2);

#endif

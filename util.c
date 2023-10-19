#include "util.h"

void tokenize(char* input, char*** tokens, int* tokens_count) {
    *tokens_count = 0;
    int token_capacity = 2;
    
    char *token = strtok(input, " \t\n\r");
    (*tokens) = (char**)malloc(sizeof(char*) * token_capacity);
    while (token != NULL) {
        if (*tokens_count >= token_capacity) {
            token_capacity *= 2;
            (*tokens) = (char**)realloc(*tokens, sizeof(char*) * token_capacity);
        }
        
        (*tokens)[*tokens_count] = token;
        (*tokens_count)++;
        token = strtok(NULL, " \t\n\r");
    }
    memset(&((*tokens)[*tokens_count]), 0, (token_capacity - *tokens_count) * sizeof(char*));
}

int compare_filenames(const void* a, const void* b) {
    const char* filename1 = *(const char**)a;
    const char* filename2 = *(const char**)b;

    return strcmp(filename1, filename2);
}

void throw_error(const char* message){
    fprintf(stderr, "%s", message);
    exit(EXIT_FAILURE);
}

char* strjoin(char* str1, const char* str2) {
    int output_len = strlen(str1) + strlen(str2);
    char* output = (char*)malloc(output_len);
    sprintf(output, "%s/%s", str1, str2);
    return output;
}
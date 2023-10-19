#include "Command.h"

Command* init_command() {
    Command* command = (Command*)malloc(sizeof(Command));
    memset(command, 0, sizeof(Command));
    command->tokens = (char**)malloc(sizeof(char*));
    return command;
}

Command* init_command_from_str(char* str) {
    Command* command = init_command();
    tokenize(str, &command->tokens, &command->tokens_count);
    return command;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "Command.h"
#include "util.h"
#include <ulimit.h>


#define SHELL_NAME "shell"            // имя файла оболочки
#define MAX_INPUT_SIZE 1024           // максимальный размер входной строки

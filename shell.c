#include "shell.h"

extern char** __environ;

void execute_external_command(Command* command) {
    
    int token_count = command->tokens_count;
    char** tokens = command->tokens;

    bool background = false;    // флаг для определения того, будет ли задача выполняться в фоне
    char* input_file = NULL;    // имя файла, на который переопределен ввод
    char* output_file = NULL;   // имя файла, на который переопределен вывод
    bool append_output = false; // флаг для определения того, нужно ли добавлять к файлу данные

    // ищем "&" в токенах, если есть - задача фоновая
    if (token_count > 0 && strcmp(tokens[token_count-1], "&") == 0) {
        background = true;
        tokens[token_count-1] = NULL;
    }

    // ищем операторы переопределения ввода/вывода
    // если такие есть, то записываем имя файла и заменяем оператор на NULL (чтобы exec* его игнорировал)
    for (int i = 0; i < token_count-1; i++) {
        if (!input_file && strcmp(tokens[i], "<") == 0) {
            if (i < token_count-1) {    
                input_file = tokens[i+1];
                tokens[i] = NULL;
            } else {
                fprintf(stderr, "Input file is missing!\n");
                return;
            }
        } else if (!output_file && strcmp(tokens[i], ">") == 0) {
            if (i < token_count-1) {
                output_file = tokens[i+1];
                tokens[i] = NULL;
            } else {
                fprintf(stderr, "Output file is missing!\n");
                return;
            }
        } else if (!output_file && strcmp(tokens[i], ">>") == 0) {
            if (i < token_count-1) {
                output_file = tokens[i+1];
                tokens[i] = NULL;
                append_output = true;
            } else {
                fprintf(stderr, "Output file is missing!\n");
                return;
            }
        }
    }

    

    // форкаемся
    
    pid_t child_pid = fork();



    
            

    if (child_pid == 0) { // дочерний процесс
        if (input_file) { // определен входной файл
            int input_fd = open(input_file, O_RDONLY);

            if (input_fd < 0)
                throw_error("Cannot open the input file!\n");

            // переопределяем стандартный ввод
            if (dup2(input_fd, STDIN_FILENO) == -1) {
                fprintf(stderr, "Cannot redirect input!\n");
            }

            close(input_fd);
        }

        if (output_file) {
            int output_fd;
            
            if (append_output) { // нашли оператор ">>"
                output_fd = open(output_file, O_CREAT | O_RDWR | O_APPEND, 0644);
            } else {
                output_fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            }
            
            if (output_fd < 0)
                throw_error("Cannot open output file!\n");

            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                fprintf(stderr, "Cannot redirect output!\n");
            }
            
            close(output_fd);
        }

        if (background) { // фоновая задача
            int dev_null_fd = open("/dev/null", O_RDWR);
            
            if (dev_null_fd < 0)
                throw_error("Cannot access /dev/null!\n");

            if (!input_file) {
                dup2(dev_null_fd, STDIN_FILENO);
            }

            if (!output_file) {
                dup2(dev_null_fd, STDOUT_FILENO);
            }

            dup2(dev_null_fd, STDERR_FILENO);

            close(dev_null_fd);
        }

        execvp(command->tokens[0], command->tokens);
        // throw_error("Cannot execute the help command!\n");
    } else if (child_pid > 0) { // родительский процесс
        if (!background) {               // если задача не фоновая, то
            waitpid(child_pid, NULL, 0); // ждем ее завершения
        }
    } else {
        fprintf(stderr, "Cannot fork the process!\n");
    }
}

// Обработка команды help
int execute_internal_help() {
    pid_t external_pid = fork();

    if(external_pid == 0) {

        int pipefd[2];

        if (pipe(pipefd) == -1) {
            fprintf(stderr, "Pipe create error\n");
            exit(EXIT_FAILURE);
        }

        pid_t child_pid = fork();

        if (child_pid == -1) {
            fprintf(stderr, "Help fork error\n");
            exit(EXIT_FAILURE);
        }

        if (child_pid == 0) {
            close(STDOUT_FILENO);

            int new_fd = dup(pipefd[1]);
            if (new_fd == -1) {
                fprintf(stderr, "dup in help error\n");
                kill(getppid(), SIGTERM);
                exit(EXIT_FAILURE);
            }

            close(pipefd[0]);
            close(pipefd[1]);

            int fd = open("README", O_RDONLY);
            if (fd == -1) {
                fprintf(stderr, "README open error\n");
                kill(getppid(), SIGTERM);
                exit(EXIT_FAILURE);
            }

            struct stat file_stat;
            if (fstat(fd, &file_stat) == -1) {
                fprintf(stderr, "Fstat in help error\n");
                close(fd);
                kill(getppid(), SIGTERM);
                exit(EXIT_FAILURE);
            }

            char buffer[file_stat.st_size];
            ssize_t n;
            while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
                ssize_t written = write(STDOUT_FILENO, buffer, n);
                if (written == -1) {
                    fprintf(stderr, "Write in help error\n");
                    kill(getppid(), SIGTERM);
                    exit(EXIT_FAILURE);
                }
            }

            close(fd);

            exit(EXIT_SUCCESS);
        } else {
            close(pipefd[1]);

            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execlp("less", "less", (char *)NULL);
            exit(EXIT_FAILURE);
        }
    } else {
        waitpid(external_pid, NULL, 0);
        return true;
    }
    return true;
}

void execute_internal_cd(Command* command) {
    if (command->tokens_count == 1) { // дана только сама команда
        if (chdir(".") != 0)
            fprintf(stderr, "Cannot change directory!\n");
    } else {
        if (chdir(command->tokens[1]) != 0) {
            fprintf(stderr, "Cannot change directory!\n");
        } else { // меняем текущую рабочую папку
            setenv("PWD", getcwd(NULL, 0), 1);
        }
    }
}

void execute_internal_clr(Command* command) {
    printf("\x1B[2J"); // очистка экрана
    printf("\x1B[H");  // перенос курсора в начало
}

bool execute_internal_dir(Command* command) {
    struct dirent* entry;                                              // структура, описывающая директорию
    DIR* dir = NULL;
    if (command->tokens_count >= 1)
    {
        dir = opendir("."); // если дана только команда - ничего не меняем
    }
        
    else
        dir = opendir(command->tokens[1]);
    
    // if (dir == NULL) {
    //     fprintf(stderr, "Cannot open directory!\n");
    //     return true;
    // }

    // проходимся по папке
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return true;
}

void execute_internal_environ(Command* command) {
    char** env = __environ;
    while (*env) {
        printf("%s\n", *env);
        env++;
    }
}

void execute_internal_echo(char** tokens, int tokens_count) {
    for (int i = 1; i < tokens_count; i++) {

        if (i + 1 < tokens_count)
        {
            if (tokens[i + 1] == NULL)
            {
                if (tokens[i] != NULL)
                    printf("%s%s", tokens[i],"\n");
            }
            else
            {
                if (tokens[i] != NULL)
                printf("%s%s", tokens[i]," ");
            }
        }
        else
            if (tokens[i] != NULL)
                printf("%s%s", tokens[i],"\n");
            else
                break;

        // printf("%s%s", command->tokens[i], ((i == command->tokens_count-1) ? "\n" : " "));|

        // is_null = command->tokens[i] == NULL;
        // if (!is_null)
        //     printf("%s%s", command->tokens[i], ((i == command->tokens_count-1) ? "\n" : " "));
        // else
        //     printf("%s%s", command->tokens[i], " ");
        // // if (command->tokens[i] == NULL)
        // //     break;
    }
}

void execute_internal_pause(Command* command) {
    printf("Press Enter to continue...");
    getchar();
}

void execute_internal_quit_exit(Command* command) {
    exit(EXIT_SUCCESS);
}

bool execute_internal_command(Command* command) {
    if (command->tokens_count == 0)
        return true;
    
    char** tokens = (char**)malloc(sizeof(char*) * command->tokens_count);
    memcpy(tokens, command->tokens, sizeof(char*) * command->tokens_count);
    
    bool background = false;    // флаг для определения того, будет ли задача выполняться в фоне
    char* input_file = NULL;    // имя файла, на который переопределен ввод
    char* output_file = NULL;   // имя файла, на который переопределен вывод
    bool append_output = false; // флаг для определения того, нужно ли добавлять к файлу данные

    if (command->tokens_count > 0 && strcmp(tokens[command->tokens_count-1], "&") == 0) {
        background = true;
        tokens[command->tokens_count-1] = NULL;
    }

        for (int i = 0; i < command->tokens_count-1; i++) {
        if (!input_file && strcmp(tokens[i], "<") == 0) {
            if (i < command->tokens_count-1) {    
                input_file = tokens[i+1];
                tokens[i] = NULL;
                tokens[i + 1] = NULL;
            } else {
                fprintf(stderr, "Input file is missing!\n");
                return true;
            }
        } else if (!output_file && strcmp(tokens[i], ">") == 0) {
            if (i < command->tokens_count-1) {
                output_file = tokens[i+1];
                tokens[i] = NULL;
                tokens[i + 1] = NULL;
            } else {
                fprintf(stderr, "Output file is missing!\n");
                return true;
            }
        } else if (!output_file && strcmp(tokens[i], ">>") == 0) {
            if (i < command->tokens_count-1) {
                output_file = tokens[i+1];
                tokens[i] = NULL;
                tokens[i + 1] = NULL;
                append_output = true;
            } else {
                fprintf(stderr, "Output file is missing!\n");
                return true;
            }
        }
    }

    if (input_file) { // определен входной файл
            int input_fd = open(input_file, O_RDONLY);

            if (input_fd < 0)
                throw_error("Cannot open the input file!\n");

            // переопределяем стандартный ввод
            if (dup2(input_fd, STDIN_FILENO) == -1) {
                fprintf(stderr, "Cannot redirect input!\n");
            }

            close(input_fd);
        }

        if (output_file) {
            int output_fd;
            
            if (append_output) { // нашли оператор ">>"
                output_fd = open(output_file, O_CREAT | O_RDWR | O_APPEND, 0644);
            } else {
                output_fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            }
            
            if (output_fd < 0)
                throw_error("Cannot open output file!\n");

            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                fprintf(stderr, "Cannot redirect output!\n");
            }
            
            close(output_fd);
        }

        if (background) { // фоновая задача
            int dev_null_fd = open("/dev/null", O_RDWR);
            
            if (dev_null_fd < 0)
                throw_error("Cannot access /dev/null!\n");

            if (!input_file) {
                dup2(dev_null_fd, STDIN_FILENO);
            }

            if (!output_file) {
                dup2(dev_null_fd, STDOUT_FILENO);
            }

            dup2(dev_null_fd, STDERR_FILENO);

            close(dev_null_fd);
        }
    
    


    if (strcmp(tokens[0], "cd") == 0) {
        execute_internal_cd(command);
        return true;
    } else if (strcmp(tokens[0], "clr") == 0) {
        execute_internal_clr(command);
        return true;
    } else if (strcmp(tokens[0], "dir") == 0) {
        return execute_internal_dir(command);
    } else if (strcmp(tokens[0], "environ") == 0) {
        execute_internal_environ(command);
        return true;
    } else if (strcmp(tokens[0], "echo") == 0) {
        execute_internal_echo(tokens, command->tokens_count);
        return true;
    } else if (strcmp(tokens[0], "pause") == 0) {
        execute_internal_pause(command);
        return true;
    } else if (strcmp(tokens[0], "help") == 0) {
        return execute_internal_help();
        // return help();
    } else if (strcmp(tokens[0], "quit") == 0 || strcmp(tokens[0], "exit") == 0)
        execute_internal_quit_exit(command);
    return false;

    
    return true;
}

int execute_command(Command* command) {
    if (command->tokens_count > 0) {
        if (execute_internal_command(command))
            return 0;
        else
        {
            execute_external_command(command);
            return 0;
        }
    }
    return 1;
}

int execute_commands_from_file(char* path) {
    FILE* cmd_source = fopen(path, "r");
    if (cmd_source == NULL)
        throw_error("Cannot open commands file!\n");

    char user_input[MAX_INPUT_SIZE];

    while (fgets(user_input, sizeof(user_input), cmd_source) != NULL) {
        Command* command = init_command_from_str(user_input);
        execute_command(command);
    }

    fclose(cmd_source);

    return EXIT_SUCCESS;
}

char* get_current_working_directory() {
    char* cwd = getcwd(NULL, 0);
    if (cwd == NULL)
        throw_error("Cannot get current directory!\n");
    return cwd;
}

void print_current_working_directory(const char* ending) {
    char* current_working_dir = get_current_working_directory();
    printf("%s%s", current_working_dir, ending);
    free(current_working_dir);
}

void execute_commands_from_user_input() {
    char user_input[MAX_INPUT_SIZE];
    while (true) {
        int console_input_stream = dup(fileno(stdin));
        int console_output_stream = dup(fileno(stdout));
        int console_error_stream = dup(fileno(stderr));
        print_current_working_directory("> ");

        // если не можем получить входную строку или достигнут конец файла
        if (!fgets(user_input, MAX_INPUT_SIZE, stdin))
            break;

        Command* command = init_command_from_str(user_input);
        execute_command(command);
        free(command);
        dup2(console_input_stream, STDIN_FILENO);
        dup2(console_output_stream, STDOUT_FILENO);
        dup2(console_error_stream, STDERR_FILENO);
    }
}

int main(int argc, char* argv[]) {
    setenv("PWD", getcwd(NULL, 0), 1);
    
    if (argc > 1)
        return execute_commands_from_file(argv[1]);
    
    // получаем текущую рабочую папку
    char* cwd = get_current_working_directory();
    char* shell_path = strjoin(cwd, SHELL_NAME);
    setenv("SHELL", shell_path, 1);
    free(cwd);
    free(shell_path);

    execute_commands_from_user_input();
    return EXIT_SUCCESS;
}

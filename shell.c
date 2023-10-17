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

            if (input_fd < 0) {
                // fprintf(stderr, "Cannot open input file!\n");
                // exit(EXIT_FAILURE);
                throw_error("Cannot open input file!\n");
            }

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

        if (strcmp(tokens[0], "help") == 0) {
            execlp("less", "less", "README", NULL);
            throw_error("Cannot exec help!\n");
        } else {
            execvp(tokens[0], tokens);
            throw_error("Cannot exec help!\n");
        }
    } else if (child_pid > 0) { // родительский процесс
        if (!background) {               // если задача не фоновая, то
            waitpid(child_pid, NULL, 0); // ждем ее завершения
        }
    } else {
        fprintf(stderr, "Cannot fork process!\n");
    }
}


void execute_internal_cd(Command* command) {
    if (command->tokens_count == 1) { // дана только сама команда
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
    DIR* dir = (command->tokens_count == 1 ? opendir(".") : opendir(command->tokens[1])); // если дана только команда - ничего не меняем

    if (dir == NULL) {
        fprintf(stderr, "Cannot open directory!\n");
        return true;
    }

    char* filenames[MAX_DIRECTORY_CONTENTS]; // имена файлов и папок
    int files_cnt = 0;                       // количество файлов и папок

    // проходимся по папке
    while ((entry = readdir(dir)) != NULL) {
        filenames[files_cnt++] = strdup(entry->d_name);
    }

    closedir(dir);

    // сортируем результат
    qsort(filenames, files_cnt, sizeof(char*), compare_filenames);
    for (int i = 0; i < files_cnt; i++) {
        printf("%s\n", filenames[i]);
        free(filenames[i]);
    }

    return true;
}

void execute_internal_environ(Command* command) {
    char** env = __environ;
    while (*env) {
        printf("%s\n", *env);
        env++;
    }
}

void execute_internal_echo(Command* command) {
    for (int i = 1; i < command->tokens_count; i++) {
        printf("%s%s", command->tokens[i], ((i == command->tokens_count-1) ? "\n" : " "));
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

    if (strcmp(command->tokens[0], "cd") == 0) {
        execute_internal_cd(command);
        return true;
    } else if (strcmp(command->tokens[0], "clr") == 0) {
        execute_internal_clr(command);
        return true;
    } else if (strcmp(command->tokens[0], "dir") == 0) {
        return execute_internal_dir(command);
    } else if (strcmp(command->tokens[0], "environ") == 0) {
        execute_internal_environ(command);
        return true;
    } else if (strcmp(command->tokens[0], "echo") == 0) {
        execute_internal_echo(command);
        return true;
    } else if (strcmp(command->tokens[0], "pause") == 0) {
        execute_internal_pause(command);
        return true;
    } else if (strcmp(command->tokens[0], "quit") == 0 || strcmp(command->tokens[0], "exit") == 0)
        execute_internal_quit_exit(command);

    return false;
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
        print_current_working_directory("> ");

        // // если не можем получить входную строку или достигнут конец файла
        if (!fgets(user_input, MAX_INPUT_SIZE, stdin))
            break;

        Command* command = init_command_from_str(user_input);
        execute_command(command);
        free(command);
    }
}

int main(int argc, char* argv[]) {
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

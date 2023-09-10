#include "fib.h"

// Function to read user input
char *read_user_input() {
    char *input = (char *)malloc(MAX_INPUT_SIZE * sizeof(char));
    if (input == NULL) {
        perror("Memory allocation error");
        exit(1);
    }
    if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
        free(input);
        return NULL;
    }
    return input;
}
// Structure to store command history entry with additional details
struct CommandEntry {
    char command[MAX_INPUT_SIZE];
    pid_t pid;
    time_t start_time;
    double duration;
};
struct CommandEntry history[HISTORY_SIZE];
int history_count = 0;

// Function to add a command to the history
void addToHistory(const char *command, pid_t pid, time_t start_time, double duration) {
    if (history_count < HISTORY_SIZE) {
        struct CommandEntry entry;
        strncpy(entry.command, command, sizeof(entry.command));
        entry.pid = pid;
        entry.start_time = start_time;
        entry.duration = duration;
        history[history_count] = entry;
        history_count++;
    } else {
        // Shift older entries to make space for the new command
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            history[i] = history[i + 1];
        }
        struct CommandEntry entry;
        strncpy(entry.command, command, sizeof(entry.command));
        entry.pid = pid;
        entry.start_time = start_time;
        entry.duration = duration;
        history[HISTORY_SIZE - 1] = entry;
    }
}
// Function to display command history
void displayHistory() {
    printf("Command history:\n");
    for (int i = 0; i < history_count; i++) {
        struct CommandEntry entry = history[i];
        printf("%d) PID: %d, Start Time: %s, Duration: %.2f ms - %s\n",
               i + 1, entry.pid, ctime(&entry.start_time), entry.duration, entry.command);
    }
}

// Function to create an empty file if it doesn't exist
int touch(const char *filename) {
    FILE *file = fopen(filename, "a"); // Open the file in append mode
    if (file == NULL) {
        return -1; // Error opening the file
    }
    if (fclose(file) == EOF) {
        perror("fclose");
        return -1; // Error closing the file
    }
    fclose(file);
    return 0;
}
// Function to count lines in a file
int count_lines_in_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("count_lines_in_file");
        return -1; // Error opening the file
    }

    int line_count = 0;
    int ch;
    int in_line = 0; // Flag to check if we are inside a line

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            if (in_line) {
                line_count++;
                in_line = 0;
            }
        } else {
            in_line = 1;
        }
    }

    if (in_line) {
        // If the last line in the file does not end with a newline
        line_count++;
    }

    fclose(file);
    return line_count;
}

// Function to count lines containing a specific word in a file
int count_lines_with_word(const char *filename, const char *word) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("count_lines_with_word");
        return -1; // Error opening the file
    }

    int line_count = 0;
    char line[MAX_INPUT_SIZE];

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, word) != NULL) {
            line_count++;
        }
    }

    fclose(file);
    return line_count;
}

// Function to search for a string in a file
void search_string_in_file(const char *filename, const char *search_string) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("search_string_in_file");
        return; // Error opening the file
    }

    char line[MAX_INPUT_SIZE];

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, search_string) != NULL) {
            printf("%s", line);
        }
    }

    fclose(file);
}


// Function to execute a command using execvp
void execute_command(char *command) {
    printf("%s",command);
    pid_t pid;
    int status;

    pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process

        // Tokenize the command to separate the command and its arguments
        char *args[] = {"/bin/sh", "-c", command, NULL};
        execvp(args[0], args); // +1 for NULL

        // Handle the case where execvp fails
        perror("execvp");
        exit(1);
    } else {
        // Parent process

        // Wait for the child process to finish
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("Exit status: %d\n", WEXITSTATUS(status));
        } else {
            printf("Abnormal termination\n");
        }
    }
}

// Function to handle built-in commands
int launch(char *command) {
    // Tokenize the input to check for built-in commands
    char *args[MAX_INPUT_SIZE / 2 + 1]; // +1 for NULL
    int argc = 0;

    char *token = strtok(command, " ");
    while (token != NULL) {
        args[argc] = token;
        token = strtok(NULL, " ");
        argc++;
    }
    args[argc] = NULL; // Null-terminate the argument list

    // Check for built-in commands
    if (strcmp(args[0], "cd") == 0) {
        if (argc < 2) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1; // Command handled
    } else if (strcmp(args[0], "exit") == 0) {
        displayHistory();
        exit(0); // Exit the shell
    } else if (strcmp(args[0], "ls") == 0) {
        // Handle the 'ls' command
        if (argc > 1 && strcmp(args[1], "-l") == 0) {
            // Handle 'ls -l' command (long format)
            execute_command("ls -l");
        } else if (argc > 1 && strcmp(args[1], "-r") == 0) {
            // Handle 'ls -r' command (recursive)
            execute_command("ls -r");
        } else if (argc > 1 && strcmp(args[1], "-R") == 0) {
            // Handle 'ls -R' command (recursive)
            execute_command("ls -R");
        } else if (argc > 1 && strcmp(args[1], "/home") == 0) {
            execute_command("ls /home");
        } else {
            // Handle 'ls' command (normal)
            execute_command("ls");
        }
        return 1; // Command handled
    }else if (strcmp(args[0], "cat") == 0 && argc == 3 && strcmp(args[1], "fib.c") == 0 && strcmp(args[2], "|") == 0 && strcmp(args[3], "wc") == 0 && strcmp(args[4], "-l") == 0) {
        // Handle 'cat fib.c | wc -l' command
        int line_count = count_lines_in_file("fib.c");
        if (line_count >= 0) {
            printf("%d\n", line_count);
        } else {
            fprintf(stderr, "wc: error counting lines in file 'fib.c'\n");
        }
        return 1;
    } else if (strcmp(args[0], "cat") == 0 && argc == 5 && strcmp(args[1], "helloworld.c") == 0 && strcmp(args[2], "|") == 0 && strcmp(args[3], "grep") == 0 && strcmp(args[4], "print") == 0 && strcmp(args[5], "|") == 0 && strcmp(args[6], "wc") == 0 && strcmp(args[7], "-l") == 0) {
        // Handle 'cat helloworld.c | grep print | wc -l' command
        int line_count = count_lines_with_word("helloworld.c", "print");
        printf("%d\n", line_count);
        return 1;
    } else if (strcmp(args[0], "grep") == 0 && argc == 3 && strcmp(args[1], "printf") == 0 && strcmp(args[2], "helloworld.c") == 0) {
        // Handle 'grep printf helloworld.c' command
        search_string_in_file("helloworld.c", "printf");
        return 1;
    // }else if (argc == 3 && strcmp(args[0], "./fib") == 0) {
    
    } else if (strcmp(args[0], "./fib") == 0) {
        pid_t pid = fork(); // Create a new process

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            // This is the child process
            execvp(args[0], args); // Execute ./fib with its arguments
            perror("execvp"); // Print an error message if execvp fails
            exit(1); // Exit the child process
        } else {
            // This is the parent process
            int status;
            waitpid(pid, &status, 0); // Wait for the child process to finish
            return 1; // Command handled
        }
    }
    else if (strcmp(args[0], "./helloworld") == 0) {
        // Execute the './helloworld' program
        int status = system(args[0]); // Run the program with its name as the command
        if (status == 0) {
            putchar('\n');
        } else {
            fprintf(stderr, "Failed to execute ./helloworld\n");
        }
        return 1; // Command handled
    } else if (strcmp(args[0], "echo") == 0) {
        // Implement the echo functionality here
        for (int i = 1; i < argc; i++) {
            printf("%s ", args[i]);
        }
        printf("\n");
        return 1; // Command handled
    } else if (strcmp(args[0], "wc") == 0) {
        if (argc < 3) {
            fprintf(stderr, "wc: missing file argument\n");
        } else if (strcmp(args[1], "-l") == 0) {
            // Handle the 'wc -l' command
            int line_count = count_lines_in_file(args[2]);
            if (line_count >= 0) {
                printf("%d\n%s\n", line_count, args[2]);
            } else {
                fprintf(stderr, "wc: error counting lines in file '%s'\n", args[2]);
            }
        } else if (argc == 3 && strcmp(args[1], "-c") == 0) {
            // Handle the 'wc -c' command
            FILE *file = fopen(args[2], "r");
            if (file == NULL) {
                fprintf(stderr, "wc: cannot open file '%s'\n", args[2]);
            } else {
                int character_count = 0;
                int ch;
                while ((ch = fgetc(file)) != EOF) {
                    character_count++;
                }
                fclose(file);
                printf("%d\n%s\n", character_count, args[2]);
            }
        } else {
            fprintf(stderr, "wc: invalid usage\n");
        }
        return 1; // Command handled
    } else if (strcmp(args[0], "rm") == 0) {
        if (argc < 2) {
            fprintf(stderr, "rm: missing file operand \n");
        } else {
            for (int i = 1; i < argc; i++) {
                if (remove(args[i]) != 0) {
                    fprintf(stderr, "rm: cannot remove '%s' : No such file or directory\n", args[i]);
                }
            }
        }
        return 1;
    } else if (strcmp(args[0], "cat") == 0) {
        // Handle the 'cat' command
        if (argc < 2) {
            fprintf(stderr, "cat: missing file argument\n");
        } else {
            for (int i = 1; i < argc; i++) {
                FILE *file = fopen(args[i], "r");
                if (file == NULL) {
                    fprintf(stderr, "cat: cannot open file '%s'\n", args[i]);
                } else {
                    int ch;
                    while ((ch = fgetc(file)) != EOF) {
                        putchar(ch);
                    }
                    fclose(file);
                    putchar('\n');
                }
            }
        }
        return 1; // Command handled
    } else if (strcmp(args[0], "uniq") == 0) {
        // Handle the 'uniq' command
        if (argc < 2) {
            fprintf(stderr, "uniq: missing file argument\n");
        } else {
            FILE *file = fopen(args[1], "r");
            if (file == NULL) {
                fprintf(stderr, "uniq: cannot open file '%s'\n", args[1]);
            } else {
                char current_line[MAX_INPUT_SIZE];
                char previous_line[MAX_INPUT_SIZE];

                // Read the first line
                if (fgets(previous_line +'\n', sizeof(previous_line), file) == NULL) {
                    fclose(file);
                    return 1; // Empty file
                }

                // Print the first line since there's nothing to compare it to yet
                printf("%s", previous_line);

                // Read and compare subsequent lines
                while (fgets(current_line, sizeof(current_line), file) != NULL) {
                    if (strcmp(current_line, previous_line) != 0) {
                        // If the current line is different from the previous line, print it
                        printf("%s", current_line);
                        // Update the previous line to the current line for the next comparison
                        strcpy(previous_line, current_line);
                    }
                }

                fclose(file);
            }
        }
        return 1; // Command handled
    } else if (strcmp(args[0], "grep") == 0) {
        // Handle the 'grep' command
        if (argc < 2) {
            fprintf(stderr, "grep: missing search pattern\n");
        } else if (argc < 3) {
            fprintf(stderr, "grep: missing file argument\n");
        } else {
            char grep_command[MAX_INPUT_SIZE];
            snprintf(grep_command, sizeof(grep_command), "grep %s %s", args[1], args[2]);
            execute_command(grep_command);
        }
        return 1; // Command handled
    } else if (strcmp(args[0], "history") == 0) {
        // Display command history
        displayHistory();
        return 1; // Command handled
    } else if (strcmp(args[0], "sort") == 0) {
        // Handle the 'sort' command
        if (argc < 2) {
            fprintf(stderr, "sort: missing file argument\n");
        } else {
            FILE *file = fopen(args[1], "r");
            if (file == NULL) {
                fprintf(stderr, "sort: cannot open file '%s'\n", args[1]);
            } else {
                char lines[MAX_INPUT_SIZE][MAX_INPUT_SIZE];
                int line_count = 0;

                // Read lines from the file
                while (fgets(lines[line_count], sizeof(lines[0]), file) != NULL) {
                    line_count++;
                }

                // Sort the lines
                for (int i = 0; i < line_count - 1; i++) {
                    for (int j = 0; j < line_count - i - 1; j++) {
                        if (strcmp(lines[j], lines[j + 1]) > 0) {
                            char temp[MAX_INPUT_SIZE];
                            strcpy(temp, lines[j]);
                            strcpy(lines[j], lines[j + 1]);
                            strcpy(lines[j + 1], temp);
                        }
                    }
                }

                // Print the sorted lines
                for (int i = 0; i < line_count; i++) {
                    printf("%s", lines[i]);
                }

                fclose(file);
            }
        }
        return 1; // Command handled
    } else if (strcmp(args[0], "wc") == 0 && strcmp(args[1], "-l") == 0) {
        // Handle the 'wc -l' command
        if (argc < 3) {
            fprintf(stderr, "wc: missing file argument\n");
        } else {
            // Redirect stdout to a pipe
            int pipefd[2];
            pipe(pipefd);
            pid_t pid = fork();

            if (pid < 0) {
                perror("Fork failed");
                exit(1);
            } else if (pid == 0) {
                // Child process
                close(pipefd[0]); // Close the read end of the pipe
                dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the pipe
                close(pipefd[1]); // Close the write end of the pipe

                // Execute 'cat fib.c' and send its output to the pipe
                execlp("cat", "cat", "fib.c", NULL);
                perror("execlp");
                exit(1);
            } else {
                // Parent process
                close(pipefd[1]); // Close the write end of the pipe
                dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to the pipe
                close(pipefd[0]); // Close the read end of the pipe

                // Execute 'wc -l' and read its output from the pipe
                execlp("wc", "wc", "-l", NULL);
                perror("execlp");
                exit(1);
            }
        }
    } else if (strcmp(args[0], "pwd") == 0) {
        // Handle the 'pwd' command
        char pwd[MAX_INPUT_SIZE];
        if (getcwd(pwd, sizeof(pwd)) != NULL) {
            printf("Current directory: %s\n", pwd);
        } else {
            perror("getcwd");
        }
        return 1; // Command handled
    } else if (strcmp(args[0], "cat") == 0 && strcmp(args[1], "fib.c") == 0 && strcmp(args[2], "|") == 0 && strcmp(args[3], "wc") == 0 && strcmp(args[4], "-l") == 0) {
        // Handle the 'cat fib.c | wc -l' command
        int pipefd[2];
        pipe(pipefd);

        pid_t cat_pid = fork();

        if (cat_pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (cat_pid == 0) {
            // Child process for 'cat'
            close(pipefd[0]); // Close the read end of the pipe
            dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the pipe
            close(pipefd[1]); // Close the write end of the pipe

            // Execute 'cat fib.c' and send its output to the pipe
            execlp("cat", "cat", "fib.c", NULL);
            perror("execlp cat");
            exit(1);
        } else {
            // Parent process
            wait(NULL); // Wait for the 'cat' process to finish
            close(pipefd[1]); // Close the write end of the pipe
            dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to the pipe
            close(pipefd[0]); // Close the read end of the pipe

            // Execute 'wc -l' and read its output from the pipe
            execlp("wc", "wc", "-l", NULL);
            perror("execlp wc");
            exit(1);
        }
    }
    else if (strcmp(args[0], "touch") == 0) {
        if (argc < 2) {
            fprintf(stderr, "touch : missing file argument\n");
        } else {
            if (access(args[1], F_OK) == -1) {
                if (touch(args[1]) != 0) {
                    fprintf(stderr, "touch : unable to create file\n");
                }
            } else {
                if (utime(args[1], NULL) != 0) {
                    fprintf(stderr, "Touch: Unable to update file timestamp\n");
                }
            }
        }
        return 1;
    } else if (argc == 3 && strstr(args[0], "./fib") != NULL) {
    // Handle the './fib' command with an argument
        int n;
        if (scanf(args[1], "%d", &n) != 1) {
            fprintf(stderr, "./fib: invalid argument\n");
            return 1;
        }if (n >= 0) {
            char command[MAX_INPUT_SIZE];
            snprintf(command, sizeof(command), "./fib %d", n);
            execute_command(command);
        } else {
            fprintf(stderr, "./fib: invalid argument\n");
        }
        return 1; // Command handled
    }
    else if (strcmp(args[0], "which") == 0) {
        // Handle the 'which' command
        if (argc < 2) {
            fprintf(stderr, "which: missing command name\n");
        } else {
            // Search for the command in the directories listed in PATH
            char *path = getenv("PATH");
            if (path != NULL) {
                char *path_copy = strdup(path);
                char *dir = strtok(path_copy, ":");
                while (dir != NULL) {
                    char which_command[MAX_INPUT_SIZE];
                    snprintf(which_command, sizeof(which_command), "%s/%s", dir, args[1]);
                    if (access(which_command, X_OK) == 0) {
                        printf("%s\n", which_command);
                        break; // Command found in this directory
                    }
                    dir = strtok(NULL, ":");
                }
                free(path_copy);
            } else {
                fprintf(stderr, "which: PATH environment variable not set\n");
            }
        }
        return 1; // Command handled
    }else{
        pid_t pid = fork(); // Create a new process

        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            // This is the child process
            execvp(args[0], args); // Execute ./fib with its arguments
            perror("execvp"); // Print an error message if execvp fails
            exit(1); // Exit the child process
        } else {
            // This is the parent process
            int status;
            waitpid(pid, &status, 0); // Wait for the child process to finish
            return 1; // Command handled
        }

    }
    return 0; // Command not handled
}

void clear_screen() {
    system("clear"); // Executes the "clear" command to clear the screen
}
// Function to execute a pipeline of commands
int execute_pipeline(char *commands[], int num_pipes) {
    int pipes[num_pipes - 1][2]; // Create pipes for communication between commands
    pid_t pids[num_pipes]; // Store child process IDs

    for (int i = 0; i < num_pipes; i++) {
        if (i < num_pipes - 1) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(0);
            }
        }

        pids[i] = fork();

        if (pids[i] == -1) {
            perror("fork");
            exit(0);
        } else if (pids[i] == 0) {
            // Child process
            if (i > 0) {
                // Redirect input from the previous pipe
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }
            if (i < num_pipes - 1) {
                // Redirect output to the next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            char *token = strtok(commands[i], " ");
            char *args[MAX_INPUT_SIZE / 2 + 1];
            int argc = 0;

            while (token != NULL) {
                args[argc] = token;
                token = strtok(NULL, " ");
                argc++;
            }
            args[argc] = NULL;

            execvp(args[0], args);
            perror("execvp"); // Print an error message if execvp fails
            exit(0); // Exit the child process
        } else {
            // Parent process
            if (i > 0) {
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }
            waitpid(pids[i], NULL, 0); // Wait for the child process to finish
        }
    }
    return 1;
}
// Function to execute a command in the background
int launch_in_background(char *command) {
    // Tokenize the input to separate the command and its arguments
    char *args[MAX_INPUT_SIZE / 2 + 1]; // +1 for NULL
    int argc = 0;

    char *token = strtok(command, " ");
    while (token != NULL) {
        args[argc] = token;
        token = strtok(NULL, " ");
        argc++;
    }
    args[argc] = NULL; // Null-terminate the argument list

    // Fork a new process for background execution
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process

        // Execute the command
        execvp(args[0], args);
        perror("execvp"); // Print an error message if execvp fails
        exit(1); // Exit the child process
    } else {
        // Parent process

        // Record the start time
        time_t start_time = time(NULL);

        // Wait for the background process to finish
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("PID %d exited_status: %d\n", pid, WEXITSTATUS(status));
        } else {
            printf("PID %d terminated_abnormally\n", pid);
        }

        // Calculate the duration
        time_t end_time = time(NULL);
        double duration = difftime(end_time, start_time) * 1000; // Duration in milliseconds

        printf("PID: %d started_Time:  %s", pid, ctime(&start_time));
        printf("PID: %d duration: %.2f ms\n", pid, duration);

        return 1;
    }
}

int main() {
    clear_screen();
    int status;
    int duration;
    time_t start_time;
    int pid;
    start_time = time(NULL);
    int background_pids[MAX_INPUT_SIZE / 2 + 1]; // Store background process IDs
    int num_background_pids = 0;

   do {
        printf("Coco_Shell :~$ ");
        char *command = read_user_input();

        if (command != NULL) {
            // Remove the trailing newline character
            command[strlen(command) - 1] = '\0';
            duration = difftime(time(NULL), start_time) * 1000; // Calculate duration in milliseconds
            addToHistory(command, pid, start_time, duration); // Add command to history

            // Check if the command is to be executed in the background
            int is_background = 0;
            if (strlen(command) > 0 && command[strlen(command) - 1] == '&') {
                is_background = 1;
                command[strlen(command) - 1] = '\0'; // Remove the '&' symbol
            }

            if (strchr(command, '|') != NULL) {
                // Execute pipelined commands
                int num_pipes = 0;
                char *commands[10]; // Assuming a maximum of 10 commands in the pipeline
                char *token = strtok(command, "|");

                while (token != NULL && num_pipes < 10) {
                    commands[num_pipes] = token;
                    num_pipes++;
                    token = strtok(NULL, "|");
                }

                status = execute_pipeline(commands, num_pipes);
            } else if (is_background) {
                // Execute the command in the background
                status = launch_in_background(command);
            } else {
                // Execute a single command
                status = launch(command);
            }

            free(command); // Free the allocated memory for the command
        } else {
            status = 0;
        }
    } while (status);

    return 0;
}
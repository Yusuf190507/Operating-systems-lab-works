#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
//#include <signal.h>        если нужно будет убить процесс:    kill(pid, SIGTERM | SIGKILL)

int checker(long long *res) {
    char buffer[32];

    errno = 0;

    ssize_t bytes_read = read(STDIN_FILENO, &buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        char message[] = "Error while reading.\n";
        write(STDERR_FILENO, message, sizeof(message));
        exit(EXIT_FAILURE);         
    } else if (bytes_read == 0) {
        return -1; // EOF
    }

    buffer[bytes_read] = '\0';
    char *ptr;
    long long result = strtoll(buffer, &ptr, 10);

    if (buffer == ptr) {
        char message[] = "This is not a number, incorrect input.\n";
        write(STDERR_FILENO, message, sizeof(message));
        return 1;
    }

    if (!strcmp(ptr, "\n") && !strcmp(ptr, "\0")) { // \0 ставится, если это был конец файла
        char message[] = "There're more symbols after the number, incorrect input.\n";
        write(STDERR_FILENO, message, sizeof(message));
        return 1;
    }

    if (errno == ERANGE) {
        char message[] = "Too long number, incorrect input.\n";
        write(STDERR_FILENO, message, sizeof(message));
        return 1;
    }

    *res = result;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        char message[] = "Usage: program_name filename\n";
        write(STDERR_FILENO, message, sizeof(message));
        return 1; //ошибка ввода
    }

    // if (access(argv[1], F_OK | W_OK) != 0) {
    //     char message[] = "The file either doesn't exist or doesn't have rights to be edited.\n";
    //     write(STDERR_FILENO, message, sizeof(message));
    //     return 1;
    // }

    int parent_to_child[2];
    if (pipe(parent_to_child) == -1) {
        char message[] = "Error: failed tp spawn a pipe.\n";
        write(STDERR_FILENO, message, sizeof(message));
        return 2; //ошибка при создании дочернего процесса
    }

    pid_t pid = fork();

    if (pid < 0) {
        char message[] = "Fork error.\n";
        write(STDERR_FILENO, message, sizeof(message));
        return 2;

    } else if (pid == 0) {
        //Ребенок
        dup2(parent_to_child[0], STDIN_FILENO);
        close(parent_to_child[0]);
        close(parent_to_child[1]);
        
        execl("./child", "child", argv[1], NULL);
        char message[] = "Error while executing the file: ";
        write(STDERR_FILENO, message, sizeof(message));
        write(STDERR_FILENO, argv[1], strlen(argv[1]));
        write(STDERR_FILENO, ".\n", sizeof(".\n"));
        return 2;

    } else {
        //Родитель
        int stdout_backup = dup(STDOUT_FILENO);
        dup2(parent_to_child[1], STDOUT_FILENO);
        close(parent_to_child[0]);
        close(parent_to_child[1]);

        long long result = 0;
        int wstatus;
        pid_t flag = 0;

        while (true) {
            usleep(500);
            flag = waitpid(pid, &wstatus, WNOHANG);

            if (flag == -1) {
                char message[] = "Waitpid error.\n";
                write(STDERR_FILENO, &message, sizeof(message));
                return 2;
            } else if (flag > 0) {                        // *
                break;
            }

            int return_code = checker(&result);
            if (return_code == -1) {                      // EOF
                break;
            } 

            else if (return_code == 0) {                  // число получено
                flag = waitpid(pid, &wstatus, WNOHANG);   // проверяем ещё раз на всякий
                if (flag == -1) {
                    char message[] = "Waitpid error.\n";
                    write(STDERR_FILENO, &message, sizeof(message));
                    return 2;
                } else if (flag > 0) {                    // *
                    break;
                }
                write(STDOUT_FILENO, &result, sizeof(result)); //отправляем число напрямую
            }
        }

        dup2(stdout_backup, STDOUT_FILENO);
        close(stdout_backup);

        if (flag == 0) {                                  // обход случая *
            flag = waitpid(pid, &wstatus, 0);
        } 
        
        if (flag == -1) {
            char message[] = "Waitpid error.\n";
            write(STDERR_FILENO, &message, sizeof(message));
            return 2;
        }

        if (WIFEXITED(wstatus)) {
            //int exit_status = WEXITSTATUS(wstatus);
            char message[] = "The decendant finished successfully.\n";
            write(STDOUT_FILENO, message, sizeof(message));
            // write(STDOUT_FILENO, exit_status, sizeof(exit_status));
            //write(STDOUT_FILENO, ".\n", sizeof(".\n"));
        } else if (WIFSIGNALED(wstatus)) {
            //int sig_num = WTERMSIG(wstatus);
            char message[] = "The decendant was killed by signal.\n";
            write(STDOUT_FILENO, message, sizeof(message));
            // write(STDOUT_FILENO, sig_num, sizeof(sig_num));
            //write(STDOUT_FILENO, ".\n", sizeof(".\n"));
        } else {
            char message[] = "Something went wrong.\n";
            write(STDOUT_FILENO, message, sizeof(message));
        }
    }
    return 0;
}

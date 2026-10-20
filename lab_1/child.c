#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <stdbool.h>

int main(int argc, char *argv[]) {
    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
    if (fd == -1) {
        char message[] = "File opening error.\n";
        write(STDERR_FILENO, message, sizeof(message));
        return 1;
    }

    long long num;
    ssize_t res = 0;

    while (res = read(STDIN_FILENO, &num, sizeof(num))) {
        if (res < 0) {
            char message[] = "Error while reading.\n";
            write(STDERR_FILENO, message, sizeof(message));
            return 1; // ошибка обработки
        }

        if (num <= 3) {
            break;
        } else {
            if (num % 2 == 0 || num % 3 == 0) {
                ssize_t bytes = write(fd, &num, sizeof(num));

                if (bytes != sizeof(num)) {
                    char message[] = "File editing error.\n";
                    write(STDERR_FILENO, message, sizeof(message));
                    close(fd);
                    return 1;
                }
            } else {
                long long final = (long long)floor(sqrt((double)num)) + 1;
                bool fl = false;
                for (int i = 6; i <= final; i += 6) {
                    if (num % (i - 1) == 0 || num % (i + 1) == 0) {
                        ssize_t bytes = write(fd, &num, sizeof(num));
                        fl = true;

                        if (bytes != sizeof(num)) {
                            char message[] = "File editing error.\n";
                            write(STDERR_FILENO, message, sizeof(message));
                            close(fd);
                            return 1;
                        }
                        break;       
                    }
                }
                if (fl == false) {
                    break;
                }
            }
        }
    }

    close(fd); 

    return 0;
}

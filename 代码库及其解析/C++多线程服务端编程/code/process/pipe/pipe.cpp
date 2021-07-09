#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd[2];
    if (pipe(fd)) {
        perror("pipe");
        return 1;
    }
    // 实现父进程写，子进程读
    pid_t id = fork();
    if (id < 0) {
        perror("fork");
        return 2;
    } else if (id == 0) {// child
        close(fd[1]);

        char buf[128];
        int cnt = 0;
        while (cnt++ < 5) {
            ssize_t _s = read(fd[0], buf, sizeof(buf));
            if (_s > 0) {
                buf[_s] = '\0';
                ;
                printf("father say to child: %s\n", buf);
            } else if (_s == 0) {
                printf("father close write");
                break;
            } else {
                perror("read");
                break;
            }
        }

        close(fd[0]);
    } else {// father
        close(fd[0]);

        char *msg = "hello world";
        int cnt = 0;
        while (cnt++ < 5) {
            write(fd[1], msg, strlen(msg));
            sleep(1);
        }

        close(fd[1]);
    }

    return 0;
}
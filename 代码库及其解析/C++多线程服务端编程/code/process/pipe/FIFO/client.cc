#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd = open("./.fifo", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 2;
    }

    int cnt = 0;
    char buf[128];
    while (cnt++ < 5) {
        ssize_t _s = read(fd, buf, sizeof(buf) - 1);
        if (_s > 0) {
            buf[_s] = '\0';
            ;
            printf("server say to client: %s\n", buf);
        } else if (_s == 0) {
            printf("server close write\n");
            break;
        } else {
            perror("read");
        }
        sleep(1);
    }

    close(fd);
    return 0;
}
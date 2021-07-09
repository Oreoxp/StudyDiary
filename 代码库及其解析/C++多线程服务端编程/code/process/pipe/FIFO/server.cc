#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main()
{
    // 创建管道时需要在mode参数位置传S_IFIFO,表明创建的是命名管道
    int ret = mkfifo("./.fifo", S_IFIFO | 0644);
    if (ret < 0) {
        perror("mkfifo");
        return 1;
    }

    int fd = open("./.fifo", O_WRONLY);
    if (fd < 0) {
        perror("open");
        return 2;
    }

    int cnt = 0;
    char *msg = "hello world";
    while (cnt++ < 5) {
        write(fd, msg, strlen(msg));
        sleep(1);
    }

    close(fd);
    return 0;
}
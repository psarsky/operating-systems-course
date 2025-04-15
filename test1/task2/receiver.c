#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define PIPE "./squareFIFO"

int main() {
    int val = 0;
    int fd;

    if ((fd = open(PIPE, O_RDONLY)) == -1) {
        perror("open");
        return 1;
    }

    if (read(fd, &val, sizeof(int)) == -1) {
        perror("read");
        close(fd);
        return 1;
    }

    close(fd);

    printf("%d square is: %d\n", val, val*val);
    return 0;
}

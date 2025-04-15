#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define PIPE "./squareFIFO"

int main(int argc, char* argv[]) {

    if(argc !=2){
        printf("Not a suitable number of program parameters\n");
        return(1);
    }

    int fd;
    if ((fd = open(PIPE, O_WRONLY)) == -1) {
        if (mkfifo(PIPE, 0666) == -1) {
            perror("mkfifo");
            return 1;
        }
    }
    
    if ((fd = open(PIPE, O_WRONLY)) == -1) {
        perror("open");
        return 1;
    }

    int number = atoi(argv[1]);

    if (write(fd, &number, sizeof(int)) == -1) {
        perror("write");
        close(fd);
        return 1;
    }

    close(fd);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <semaphore.h>

#define FILE_NAME "common.txt"
#define SEM_NAME "my_kol_sem"

sem_t *sem_id;

void print_sem_value(sem_t *sem){
    int sem_val;
    if (sem_getvalue(sem,&sem_val) == -1) {
        perror("sem_getvalue failed");
        return;
    }
    printf("Current sem value: %d\n",sem_val);
}

int main(int argc, char** args){

    if(argc !=4){
        printf("Not a suitable number of program parameters\n");
        return(1);
    }

    /************************************
    Utworz semafor posixowy. Ustaw jego wartosc na 1
    *************************************/

    sem_id = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
    if (sem_id == SEM_FAILED) {
        perror("sem_open failed");
        return 1;
    }

    print_sem_value(sem_id);

    int fd = open(FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC , 0644);
    if (fd == -1) {
        perror("open failed");
        sem_close(sem_id);
        sem_unlink(SEM_NAME);
        return 1;
    }

    int parentLoopCounter = atoi(args[1]);
    int childLoopCounter = atoi(args[2]);

    char buf[200];
    pid_t childPid;
    int max_sleep_time = atoi(args[3]);

    if((childPid=fork())) {
        int status =0;
        srand((unsigned)time(0));
        while(parentLoopCounter--){
            int s = rand()%max_sleep_time+1;
            sleep(s);
            print_sem_value(sem_id);

            /******************************************
            Sekcja krytyczna. Zabezpiecz dostep semaforem
            ******************************************/
            if (sem_wait(sem_id) == -1) {
                perror("parent sem_wait failed");
                close(fd);
                sem_close(sem_id);
                sem_unlink(SEM_NAME);
                exit(1);
            }

            print_sem_value(sem_id);
            sprintf(buf, "Wpis rodzica. Petla %d. Spalem %ds.\n", parentLoopCounter,s);
            write(fd, buf, strlen(buf));
            write(1, buf, strlen(buf));
            print_sem_value(sem_id);

            /****************************************************
            Koniec sekcji krytycznej
	        ****************************************************/
            if (sem_post(sem_id) == -1) {
                perror("parent sem_post failed");
                close(fd);
                sem_close(sem_id);
                sem_unlink(SEM_NAME);
                exit(1);
            }

            print_sem_value(sem_id);
        }
        waitpid(childPid,&status,0);
    } else {
        srand((unsigned)time(0));
        while(childLoopCounter--) {
            int s = rand()%max_sleep_time+1;
            sleep(s);
            print_sem_value(sem_id);

            /******************************************
            Sekcja krytyczna. Zabezpiecz dostep semaforem
            ******************************************/
            if (sem_wait(sem_id) == -1) {
                perror("child sem_wait failed");
                close(fd);
                sem_close(sem_id);
                _exit(1);
            }

            print_sem_value(sem_id);
            sprintf(buf, "Wpis dziecka. Petla %d. Spalem %ds.\n", childLoopCounter,s);
            write(fd, buf, strlen(buf));
            write(1, buf, strlen(buf));
            print_sem_value(sem_id);

            /****************************************************
            Koniec sekcji krytycznej
            ****************************************************/
            if (sem_post(sem_id) == -1) {
                perror("child sem_post failed");
                close(fd);
                sem_close(sem_id);
                _exit(1);
            }

            print_sem_value(sem_id);
        }
        close(fd);
        sem_close(sem_id);
        _exit(0);
    }

    /***************************************
    Posprzataj semafor
    ***************************************/
    sem_close(sem_id);
    if (sem_unlink(SEM_NAME) == -1) {
        perror("sem_unlink failed");
    }

    close(fd);
    return 0;
}
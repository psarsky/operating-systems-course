#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#define MESSAGE_BUFFER_SIZE 2048
#define SERVER_QUEUE_NAME "/simple_chat_server_queue"
#define CLIENT_QUEUE_NAME_SIZE 40
#define MAX_CLIENTS_COUNT 3

typedef enum {
    INIT, 
    IDENTIFIER,
    MESSAGE_TEXT,
    CLIENT_CLOSE
} message_type_t;

typedef struct {
    message_type_t type;

    int identifier;
    char text[MESSAGE_BUFFER_SIZE];
} message_t;

volatile bool should_close = false;

void SIGNAL_handler(int signum) {
    should_close = true;
}

#define MIN(a, b) (a < b ? a : b)

int main() {
    pid_t pid = getpid();
    char queue_name[CLIENT_QUEUE_NAME_SIZE] = {0};
    sprintf(queue_name, "/simple_chat_client_queue_%d", pid);

    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_msgsize = sizeof(message_t),
        .mq_maxmsg = 10
    };

    mqd_t mq_client_descriptor = mq_open(queue_name, O_RDWR | O_CREAT,  S_IRUSR | S_IWUSR, &attributes);
    if(mq_client_descriptor < 0)
        perror("mq_open client");

    mqd_t mq_server_descriptor = mq_open(SERVER_QUEUE_NAME, O_RDWR, S_IRUSR | S_IWUSR, NULL);
    if(mq_server_descriptor < 0) {
        printf("Most likely server not opened!\n");
        perror("mq_open server");
    }

    message_t message_init = {
        .type = INIT,
        .identifier = -1
    };

    memcpy(message_init.text, queue_name, MIN(CLIENT_QUEUE_NAME_SIZE - 1, strlen(queue_name)));

    if(mq_send(mq_server_descriptor, (char*)&message_init, sizeof(message_init), 10) < 0){
        perror("mq_send init");
    }

    int to_parent_pipe[2];
    if(pipe(to_parent_pipe) < 0)
        perror("pipe");

    for (int sig = 1; sig < SIGRTMAX; sig++) {
        signal(sig, SIGNAL_handler);
    }

    pid_t listener_pid = fork();
    if (listener_pid < 0)
        perror("fork listener");
    else if (listener_pid == 0) {
        close(to_parent_pipe[0]);
        message_t receive_message;
        while(!should_close) {
            mq_receive(mq_client_descriptor, (char*)&receive_message, sizeof(receive_message), NULL);
            switch(receive_message.type) {
                case MESSAGE_TEXT:
                    printf("Received from id: %d message: %s\n", receive_message.identifier, receive_message.text);
                    break;
                case IDENTIFIER:
                    printf("Received identifier from server: %d\n", receive_message.identifier);
                    write(to_parent_pipe[1], &receive_message.identifier, sizeof(receive_message.identifier));
                    break;
                default:
                    printf("Unknown message type in client queue: %d", receive_message.type);
                    break;
            }
        }
        printf("Exiting from receive loop\n");
        exit(0);
    } else {
        close(to_parent_pipe[1]);
        int identifier = -1;

        if(read(to_parent_pipe[0], &identifier, sizeof(identifier)) < 0)
            perror("read identifier");

        char* buffer = NULL;
        while(!should_close) {
            mq_getattr(mq_server_descriptor, &attributes);
            if(attributes.mq_curmsgs >= attributes.mq_maxmsg) {
                printf("Server is busy, please wait\n");
                continue;
            }

            if(scanf("%ms", &buffer) == 1) {
                message_t send_message = {
                    .type = MESSAGE_TEXT,
                    .identifier = identifier
                };
                memcpy(send_message.text, buffer, MIN(MESSAGE_BUFFER_SIZE - 1, strlen(buffer)));

                mq_send(mq_server_descriptor, (char*)&send_message, sizeof(send_message), 10);

                free(buffer);
                buffer = NULL;
            } else
                perror("scanf input");
        }

        printf("Exiting from sending loop\n");

        if(identifier != -1){
            message_t message_close = {
                .type = CLIENT_CLOSE,
                .identifier = identifier
            };
            mq_send(mq_server_descriptor, (char*)&message_close, sizeof(message_close), 10);
        }

        mq_close(mq_server_descriptor);
        mq_close(mq_client_descriptor);

        mq_unlink(queue_name);
    }
}
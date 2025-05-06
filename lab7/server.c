#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
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

int main() {
    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_msgsize = sizeof(message_t),
        .mq_maxmsg = 10
    };

    mqd_t mq_descriptor = mq_open(SERVER_QUEUE_NAME, O_RDWR | O_CREAT,  S_IRUSR | S_IWUSR, &attributes);
    if(mq_descriptor < 0)
        perror("mq_open server");

    message_t receive_message;

    mqd_t client_queues[MAX_CLIENTS_COUNT];
    for (int i = 0; i < MAX_CLIENTS_COUNT; i++)
        client_queues[i] = -1;

    for (int sig = 1; sig < SIGRTMAX; sig++) {
        signal(sig, SIGNAL_handler);
    }

    while(!should_close) {
        mq_receive(mq_descriptor, (char*)&receive_message, sizeof(receive_message), NULL);

        switch(receive_message.type) {
            case INIT:
                int id = 0;
                while(client_queues[id] != -1 && id < MAX_CLIENTS_COUNT) id++;

                if(id == MAX_CLIENTS_COUNT){
                    printf("Max number of clients has connected, can't open another connection\n");
                    continue;
                }

                client_queues[id] = mq_open(receive_message.text, O_RDWR, S_IRUSR | S_IWUSR, NULL);
                if(client_queues[id] < 0)
                    perror("mq_open client");

                message_t send_message = {
                    .type = IDENTIFIER,
                    .identifier = id
                };

                mq_send(client_queues[id], (char*)&send_message, sizeof(send_message), 10);
                printf("Registered connection with client at id: %d\n", id);
                break;
            case MESSAGE_TEXT:
                for (int identifier = 0; identifier < MAX_CLIENTS_COUNT; identifier++){
                    if(identifier != receive_message.identifier && identifier != -1){
                        mq_send(client_queues[identifier], (char*)&receive_message, sizeof(receive_message), 10);
                    }
                }
                break;
            case CLIENT_CLOSE:
                mq_close(client_queues[receive_message.identifier]);
                client_queues[receive_message.identifier] = -1;
                printf("Closed connection with client at id: %d\n", receive_message.identifier);
                break;
            default:
                printf("Unexpected message type in server queue: %d \n", receive_message.type);
                break;
        }
    }

    printf("Exiting server\n");

    for (int i = 0; i < MAX_CLIENTS_COUNT; i++){
        if(client_queues[i] != -1)
            mq_close(client_queues[i]);
    }

    mq_close(mq_descriptor);
    mq_unlink(SERVER_QUEUE_NAME);

    return 0;
}
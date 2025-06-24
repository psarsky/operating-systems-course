#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 512

int client_socket;
int client_running = 1;
char client_name[MAX_NAME_LEN];

// Thread function to handle received messages
void* receive_messages(void* arg) {
    (void)arg;
    char buffer[MAX_MSG_LEN];
    
    while (client_running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            if (client_running) {
                printf("Connection to server was interrupted\n");
                client_running = 0;
            }
            break;
        }
        
        // Server ping
        if (strncmp(buffer, "ALIVE", 5) == 0) {
            send(client_socket, "ALIVE_RESPONSE\n", 15, 0);
            continue;
        }
        
        printf("%s", buffer);
        fflush(stdout);
    }
    
    return NULL;
}

// SIGINT handler
void signal_handler(int sig) {
    (void)sig;
    printf("\nDisconnecting from server...\n");
    client_running = 0;

    send(client_socket, "STOP\n", 5, 0);
    
    close(client_socket);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <client_name> <ip_address:port>\n", argv[0]);
        return 1;
    }
    
    if (strlen(argv[1]) >= MAX_NAME_LEN) {
        printf("Client name too long (max %d characters)\n", MAX_NAME_LEN - 1);
        return 1;
    }
    
    strcpy(client_name, argv[1]);
    
    char* colon = strchr(argv[2], ':');
    if (!colon) {
        printf("Invalid address format. Use: ip_address:port\n");
        return 1;
    }
    
    *colon = '\0';
    char* server_ip = argv[2];
    int server_port = atoi(colon + 1);
    
    if (server_port <= 0 || server_port > 65535) {
        printf("Invalid port number\n");
        return 1;
    }
    
    signal(SIGINT, signal_handler);
    
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket");
        return 1;
    }
    
    // Configure server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    // Convert IP to binary
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid IP address\n");
        close(client_socket);
        return 1;
    }

    printf("Connecting to server %s:%d\n", server_ip, server_port);
    
    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_socket);
        return 1;
    }
    
    // Send client name to server
    char name_message[MAX_NAME_LEN + 1];
    snprintf(name_message, sizeof(name_message), "%s\n", client_name);
    send(client_socket, name_message, strlen(name_message), 0);
    
    // Wait for server response
    char response[MAX_MSG_LEN];
    int bytes_received = recv(client_socket, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        printf("%s", response);
        
        // Check if server rejected connection
        if (strstr(response, "full") || strstr(response, "taken")) {
            close(client_socket);
            return 1;
        }
    }
    
    // Thread for receiving messages
    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, NULL);
    
    char input[MAX_MSG_LEN];
    while (client_running) {
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            continue;
        }
        
        if (strcmp(input, "STOP") == 0) {
            send(client_socket, "STOP\n", 5, 0);
            client_running = 0;
            break;
        }
        
        strcat(input, "\n");
        if (send(client_socket, input, strlen(input), 0) == -1) {
            perror("send");
            break;
        }
    }
    
    if (client_running) {
        send(client_socket, "STOP\n", 5, 0);
    }
    
    close(client_socket);
    pthread_cancel(receive_thread);
    
    printf("Client terminated\n");
    return 0;
}
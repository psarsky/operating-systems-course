#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#define MAX_CLIENTS 10
#define MAX_NAME_LEN 32
#define MAX_MSG_LEN 512
#define ALIVE_INTERVAL 5

// Client structure
typedef struct {
    int socket;
    char name[MAX_NAME_LEN];
    int active;
    pthread_t thread;
} client_t;

// CLient array and mutex for synchronization
client_t clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
int server_socket;
int server_running = 1;

int find_free_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            return i;
        }
    }
    return -1;
}

int find_client_by_name(const char* name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void handle_list_command(int client_socket) {
    char response[MAX_MSG_LEN] = "CLIENTS: ";
    pthread_mutex_lock(&clients_mutex);
    
    int first = 1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            if (!first) {
                strcat(response, ", ");
            }
            strcat(response, clients[i].name);
            first = 0;
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
    strcat(response, "\n");
    send(client_socket, response, strlen(response), 0);
}

void handle_2all_command(const char* sender_name, const char* message) {
    char full_message[MAX_MSG_LEN];
    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';
    
    snprintf(full_message, sizeof(full_message), "[%s] %s: %s\n", time_str, sender_name, message);
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].name, sender_name) != 0) {
            send(clients[i].socket, full_message, strlen(full_message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void handle_2one_command(const char* sender_name, const char* target_name, const char* message) {
    char full_message[MAX_MSG_LEN];
    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';
    
    snprintf(full_message, sizeof(full_message), "[%s] %s (private): %s\n", time_str, sender_name, message);
    
    pthread_mutex_lock(&clients_mutex);
    int target_index = find_client_by_name(target_name);
    if (target_index != -1) {
        send(clients[target_index].socket, full_message, strlen(full_message), 0);
    } else {
        char error_message[MAX_MSG_LEN];
        snprintf(error_message, sizeof(error_message), "Client %s not found\n", target_name);
        send(clients[find_client_by_name(sender_name)].socket, error_message, strlen(error_message), 0);
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_index) {
    pthread_mutex_lock(&clients_mutex);
    if (clients[client_index].active) {
        close(clients[client_index].socket);
        clients[client_index].active = 0;
        printf("Client %s disconnected\n", clients[client_index].name);
        memset(clients[client_index].name, 0, MAX_NAME_LEN);
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Thread function to handle client
void* handle_client(void* arg) {
    int client_index = *(int*)arg;
    free(arg);
    
    int client_socket = clients[client_index].socket;
    char buffer[MAX_MSG_LEN];
    
    while (server_running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            break;
        }
        
        buffer[strcspn(buffer, "\n")] = 0;
        
        printf("Received from %s: %s\n", clients[client_index].name, buffer);
        
        if (strncmp(buffer, "LIST", 4) == 0) {
            handle_list_command(client_socket);
        }
        else if (strncmp(buffer, "2ALL ", 5) == 0) {
            handle_2all_command(clients[client_index].name, buffer + 5);
        }
        else if (strncmp(buffer, "2ONE ", 5) == 0) {
            char* space = strchr(buffer + 5, ' ');
            if (space) {
                *space = '\0';
                char* target_name = buffer + 5;
                char* message = space + 1;
                handle_2one_command(clients[client_index].name, target_name, message);
            }
        }
        else if (strncmp(buffer, "STOP", 4) == 0) {
            break;
        }
        else if (strncmp(buffer, "ALIVE_RESPONSE", 14) == 0) {
            // Client responded to ping
            continue;
        }
    }
    
    remove_client(client_index);
    return NULL;
}

// Ping clients
void* alive_checker(void* arg) {
    (void)arg;
    while (server_running) {
        sleep(ALIVE_INTERVAL);
        
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                send(clients[i].socket, "ALIVE\n", strlen("ALIVE\n"), 0);
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    return NULL;
}

// SIGINT handler
void signal_handler(int sig) {
    (void)sig;
    printf("\nClosing server...\n");
    server_running = 0;
    close(server_socket);
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    signal(SIGINT, signal_handler);
    memset(clients, 0, sizeof(clients));
    
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Configure server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        return 1;
    }
    
    // Start listening
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_socket);
        return 1;
    }
    
    printf("Server listening on port %d\n", port);
    
    // Thread for pinging clients
    pthread_t alive_thread;
    pthread_create(&alive_thread, NULL, alive_checker, NULL);
    
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            if (server_running) {
                perror("accept");
            }
            continue;
        }
        
        // Receive client name
        char client_name[MAX_NAME_LEN];
        int bytes_received = recv(client_socket, client_name, sizeof(client_name) - 1, 0);
        if (bytes_received <= 0) {
            close(client_socket);
            continue;
        }
        client_name[bytes_received] = '\0';
        client_name[strcspn(client_name, "\n")] = 0;
        
        // Find free slot
        pthread_mutex_lock(&clients_mutex);
        int slot = find_free_slot();
        if (slot == -1) {
            pthread_mutex_unlock(&clients_mutex);
            send(client_socket, "Server full\n", strlen("Server full\n"), 0);
            close(client_socket);
            continue;
        }
        
        // Check if name is already taken
        if (find_client_by_name(client_name) != -1) {
            pthread_mutex_unlock(&clients_mutex);
            send(client_socket, "Name already taken\n", strlen("Name already taken\n"), 0);
            close(client_socket);
            continue;
        }
        
        // Add client
        clients[slot].socket = client_socket;
        strcpy(clients[slot].name, client_name);
        clients[slot].active = 1;
        pthread_mutex_unlock(&clients_mutex);
        
        printf("New client: %s (%s:%d)\n", client_name, 
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        send(client_socket, "Connection successful\n", strlen("Connection successful\n"), 0);
        
        // Create thread for client
        int* client_index = malloc(sizeof(int));
        *client_index = slot;
        pthread_create(&clients[slot].thread, NULL, handle_client, client_index);
        pthread_detach(clients[slot].thread);
    }
    
    close(server_socket);
    return 0;
}
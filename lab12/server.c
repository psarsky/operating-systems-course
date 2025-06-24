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
    struct sockaddr_in addr;
    char name[MAX_NAME_LEN];
    int active;
    time_t last_seen;
} client_t;

// Client array and mutex for synchronization
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

int find_client_by_addr(struct sockaddr_in* addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && 
            clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            clients[i].addr.sin_port == addr->sin_port) {
            return i;
        }
    }
    return -1;
}

void handle_list_command(struct sockaddr_in* client_addr) {
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
    sendto(server_socket, response, strlen(response), 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
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
            sendto(server_socket, full_message, strlen(full_message), 0, 
                    (struct sockaddr*)&clients[i].addr, sizeof(clients[i].addr));
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
        sendto(server_socket, full_message, strlen(full_message), 0, 
                (struct sockaddr*)&clients[target_index].addr, sizeof(clients[target_index].addr));
    } else {
        char error_message[MAX_MSG_LEN];
        snprintf(error_message, sizeof(error_message), "Client %s not found\n", target_name);
        int sender_index = find_client_by_name(sender_name);
        if (sender_index != -1) {
            sendto(server_socket, error_message, strlen(error_message), 0, 
                    (struct sockaddr*)&clients[sender_index].addr, sizeof(clients[sender_index].addr));
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_index) {
    if (clients[client_index].active) {
        clients[client_index].active = 0;
        printf("Client %s disconnected\n", clients[client_index].name);
        memset(clients[client_index].name, 0, MAX_NAME_LEN);
    }
}

// Thread function to handle UDP messages
void* handle_messages(void* arg) {
    (void)arg;
    char buffer[MAX_MSG_LEN];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (server_running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recvfrom(server_socket, buffer, sizeof(buffer) - 1, 0, 
                                    (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_received <= 0) {
            if (server_running) {
                continue;
            }
            break;
        }
        
        buffer[strcspn(buffer, "\n")] = 0;
        
        pthread_mutex_lock(&clients_mutex);
        int client_index = find_client_by_addr(&client_addr);
        
        // If this is a new client registration
        if (client_index == -1 && strncmp(buffer, "REGISTER ", 9) == 0) {
            char* client_name = buffer + 9;
            
            // Find free slot
            int slot = find_free_slot();
            if (slot == -1) {
                pthread_mutex_unlock(&clients_mutex);
                sendto(server_socket, "Server full\n", strlen("Server full\n"), 0, 
                        (struct sockaddr*)&client_addr, sizeof(client_addr));
                continue;
            }
            
            // Check if name is already taken
            if (find_client_by_name(client_name) != -1) {
                pthread_mutex_unlock(&clients_mutex);
                sendto(server_socket, "Name already taken\n", strlen("Name already taken\n"), 0, 
                        (struct sockaddr*)&client_addr, sizeof(client_addr));
                continue;
            }
            
            // Add client
            clients[slot].addr = client_addr;
            strcpy(clients[slot].name, client_name);
            clients[slot].active = 1;
            clients[slot].last_seen = time(NULL);
            
            printf("New client: %s (%s:%d)\n", client_name, 
                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            
            sendto(server_socket, "Connection successful\n", strlen("Connection successful\n"), 0, 
                    (struct sockaddr*)&client_addr, sizeof(client_addr));
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        
        if (client_index == -1) {
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        
        clients[client_index].last_seen = time(NULL);
        
        printf("Received from %s: %s\n", clients[client_index].name, buffer);
        
        if (strncmp(buffer, "LIST", 4) == 0) {
            pthread_mutex_unlock(&clients_mutex);
            handle_list_command(&client_addr);
        }
        else if (strncmp(buffer, "2ALL ", 5) == 0) {
            char* sender_name = clients[client_index].name;
            pthread_mutex_unlock(&clients_mutex);
            handle_2all_command(sender_name, buffer + 5);
        }
        else if (strncmp(buffer, "2ONE ", 5) == 0) {
            char* space = strchr(buffer + 5, ' ');
            if (space) {
                *space = '\0';
                char* target_name = buffer + 5;
                char* message = space + 1;
                char* sender_name = clients[client_index].name;
                pthread_mutex_unlock(&clients_mutex);
                handle_2one_command(sender_name, target_name, message);
            } else {
                pthread_mutex_unlock(&clients_mutex);
            }
        }
        else if (strncmp(buffer, "STOP", 4) == 0) {
            remove_client(client_index);
            pthread_mutex_unlock(&clients_mutex);
        }
        else if (strncmp(buffer, "ALIVE_RESPONSE", 14) == 0) {
            // Client responded to ping
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        else {
            pthread_mutex_unlock(&clients_mutex);
        }
    }
    
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
                sendto(server_socket, "ALIVE\n", strlen("ALIVE\n"), 0, 
                        (struct sockaddr*)&clients[i].addr, sizeof(clients[i].addr));
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
    
    // Create server socket - UDP
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
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
    
    printf("Server listening on port %d\n", port);
    
    // Thread for pinging clients
    pthread_t alive_thread;
    pthread_create(&alive_thread, NULL, alive_checker, NULL);
    
    // Thread for handling messages
    pthread_t message_thread;
    pthread_create(&message_thread, NULL, handle_messages, NULL);
    
    // Keep main thread alive
    while (server_running) {
        sleep(1);
    }
    
    close(server_socket);
    return 0;
}
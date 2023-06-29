#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define MAX_MESSAGE_LENGTH 256

// Global variables
int clients[MAX_CLIENTS];
char messages[MAX_CLIENTS][MAX_MESSAGE_LENGTH];
fd_set readfds;
int maxfd;

// Function to initialize the client array
void initClients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
    }
}

// Function to add a client to the chatroom
int addClient(int clientfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == -1) {
            clients[i] = clientfd;

            if (clientfd > maxfd) {
                maxfd = clientfd;
            }

            return 1;
        }
    }

    return 0;
}

// Function to remove a client from the chatroom
void removeClient(int clientfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == clientfd) {
            clients[i] = -1;
            break;
        }
    }
}

// Function to broadcast a message to all connected clients
void broadcastMessage(char * message, int sender) {
    char new_message[MAX_MESSAGE_LENGTH] = {0};

    snprintf(new_message, MAX_MESSAGE_LENGTH, "[%d] => %s", sender, message);
    // printf("Enviando mensagem no broadcast => '%s'\n", message);
    // printf("Enviando new_message no broadcast => '%s'\n", new_message);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        int clientfd = clients[i];

        if (clientfd != -1 && clientfd != sender) {
            write(clientfd, new_message, strlen(new_message));
        }
    }
}

int main() {
    int serverfd, newclientfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int activity, i, valread;
    char message[MAX_MESSAGE_LENGTH];

    // Create a socket for the server
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    // Bind the socket to the specified address and port
    if (bind(serverfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverfd, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    initClients();
    maxfd = serverfd;

    printf("Chatroom server started. Waiting for connections...\n");

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(serverfd, &readfds);

        for (i = 0; i < MAX_CLIENTS; i++) {
            int clientfd = clients[i];

            if (clientfd != -1) {
                FD_SET(clientfd, &readfds);
            }
        }

        activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity == -1) {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(serverfd, &readfds)) {
            // New connection request
            newclientfd = accept(serverfd, (struct sockaddr*)&clientAddr, &addrLen);
            if (newclientfd == -1) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            // Add the new client to the chatroom
            if (addClient(newclientfd) == 0) {
                printf("Chatroom is full. Connection rejected.\n");
                close(newclientfd);
            } else {
                printf("New client connected. Socket fd is %d\n", newclientfd);
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++) {
            int clientfd = clients[i];

            if (clientfd != -1 && FD_ISSET(clientfd, &readfds)) {
                // Client activity
                FD_CLR(clientfd, &readfds);
                valread = read(clientfd, message, MAX_MESSAGE_LENGTH);

                if (valread == 0) {
                    // Client disconnected
                    getpeername(clientfd, (struct sockaddr*)&clientAddr, &addrLen);
                    printf("Client disconnected. IP address: %s, Port: %d\n",
                           inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

                    // Remove the client from the chatroom
                    removeClient(clientfd);
                    close(clientfd);
                } else {
                    // Broadcast the message to all other clients
                    message[valread] = '\0';
                    // message[valread + strlen(message)] = '\0';
                    broadcastMessage(message, clientfd);
                }
            }
        }
    }

    return 0;
}

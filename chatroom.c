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
#define MAX_ROOMS 5
#define MAX_ROOM_NAME_LENGTH 50
#define MAX_MESSAGE_LENGTH 256

// Structure to represent a chat room
typedef struct {
    char name[MAX_ROOM_NAME_LENGTH];
    int participants[MAX_CLIENTS];
} ChatRoom;

// Global variables
int clients[MAX_CLIENTS];
ChatRoom rooms[MAX_ROOMS];
fd_set readfds;
int maxfd;

// Function to initialize the client array
void initClients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
    }
}

// Function to initialize the chat rooms
void initRooms() {
    for (int i = 0; i < MAX_ROOMS; i++) {
        strcpy(rooms[i].name, "");
        for (int j = 0; j < MAX_CLIENTS; j++) {
            rooms[i].participants[j] = -1;
        }
    }
}

// Function to find an available room index
int findAvailableRoom() {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (strcmp(rooms[i].name, "") == 0) {
            return i;
        }
    }
    return -1; // No available room found
}

// Function to add a client to a chat room
int addClientToRoom(int clientfd, int roomIndex) {
    if (roomIndex >= 0 && roomIndex < MAX_ROOMS) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (rooms[roomIndex].participants[i] == -1) {
                rooms[roomIndex].participants[i] = clientfd;

                if (clientfd > maxfd) {
                    maxfd = clientfd;
                }

                return 1;
            }
        }
    }

    return 0;
}

// Function to remove a client from a chat room
void removeClientFromRoom(int clientfd, int roomIndex) {
    if (roomIndex >= 0 && roomIndex < MAX_ROOMS) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (rooms[roomIndex].participants[i] == clientfd) {
                rooms[roomIndex].participants[i] = -1;
                break;
            }
        }
    }
}

// Function to find the room index by name
int findRoomIndexByName(const char* name) {
    for (int i = 0; i < MAX_ROOMS; i++) {
        if (strcmp(rooms[i].name, name) == 0) {
            return i;
        }
    }
    return -1; // Room not found
}

// Function to create a chat room
int createRoom(const char* name) {
    int roomIndex = findAvailableRoom();
    if (roomIndex != -1) {
        strcpy(rooms[roomIndex].name, name);
        return 1;
    }
    return 0;
}

// Function to remove a chat room
void removeRoom(int roomIndex) {
    if (roomIndex >= 0 && roomIndex < MAX_ROOMS) {
        strcpy(rooms[roomIndex].name, "");
        for (int i = 0; i < MAX_CLIENTS; i++) {
            rooms[roomIndex].participants[i] = -1;
        }
    }
}

// Function to broadcast a message to all participants in a room
void broadcastMessageToRoom(char* message, int sender, int roomIndex) {
    char new_message[MAX_MESSAGE_LENGTH] = {0};

    snprintf(new_message, MAX_MESSAGE_LENGTH, "[%d] => %s", sender, message);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        int clientfd = rooms[roomIndex].participants[i];

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
    int roomIndex = -1;

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
    initRooms();
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

            roomIndex = findAvailableRoom();
            if (roomIndex != -1) {
                strcpy(rooms[roomIndex].name, "");
            }

            if (addClientToRoom(newclientfd, roomIndex) == 0) {
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
                    removeClientFromRoom(clientfd, roomIndex);
                    close(clientfd);
                } else {
                    // Handle client message
                    message[valread] = '\0';

                    if (message[0] == '/') {
                        // Command message
                        char* command = strtok(message, " ");
                        if (strcmp(command, "/create") == 0) {
                            char* roomName = strtok(NULL, " ");
                            if (roomName != NULL) {
                                if (createRoom(roomName)) {
                                    printf("New chat room created: %s\n", roomName);
                                    write(clientfd, "Chat room created successfully.", strlen("Chat room created successfully."));
                                } else {
                                    write(clientfd, "Failed to create chat room.", strlen("Failed to create chat room."));
                                }
                            } else {
                                write(clientfd, "Invalid command syntax.", strlen("Invalid command syntax."));
                            }
                        } else if (strcmp(command, "/join") == 0) {
                            char* roomName = strtok(NULL, " ");
                            if (roomName != NULL) {
                                int roomIndex = findRoomIndexByName(roomName);
                                if (roomIndex != -1) {
                                    if (addClientToRoom(clientfd, roomIndex)) {
                                        printf("Client %d joined chat room: %s\n", clientfd, roomName);
                                        write(clientfd, "Joined chat room successfully.", strlen("Joined chat room successfully."));
                                    } else {
                                        write(clientfd, "Chat room is full.", strlen("Chat room is full."));
                                    }
                                } else {
                                    write(clientfd, "Chat room not found.", strlen("Chat room not found."));
                                }
                            } else {
                                write(clientfd, "Invalid command syntax.", strlen("Invalid command syntax."));
                            }
                        } else if (strcmp(command, "/leave") == 0) {
                            int leftRoomIndex = -1;
                            for (int j = 0; j < MAX_ROOMS; j++) {
                                for (int k = 0; k < MAX_CLIENTS; k++) {
                                    if (rooms[j].participants[k] == clientfd) {
                                        leftRoomIndex = j;
                                        break;
                                    }
                                }
                                if (leftRoomIndex != -1) {
                                    break;
                                }
                            }

                            if (leftRoomIndex != -1) {
                                removeClientFromRoom(clientfd, leftRoomIndex);
                                printf("Client %d left chat room: %s\n", clientfd, rooms[leftRoomIndex].name);
                                write(clientfd, "Left chat room successfully.", strlen("Left chat room successfully."));
                            } else {
                                write(clientfd, "You are not currently in any chat room.", strlen("You are not currently in any chat room."));
                            }
                        } else if (strcmp(command, "/list") == 0) {
                            char roomList[MAX_MESSAGE_LENGTH];
                            roomList[0] = '\0';
                            for (int j = 0; j < MAX_ROOMS; j++) {
                                if (strcmp(rooms[j].name, "") != 0) {
                                    strcat(roomList, rooms[j].name);
                                    strcat(roomList, "\n");
                                }
                            }
                            write(clientfd, roomList, strlen(roomList));
                        } else {
                            write(clientfd, "Unknown command.", strlen("Unknown command."));
                        }
                    } else {
                        // Regular message
                        int currentRoomIndex = -1;
                        for (int j = 0; j < MAX_ROOMS; j++) {
                            for (int k = 0; k < MAX_CLIENTS; k++) {
                                if (rooms[j].participants[k] == clientfd) {
                                    currentRoomIndex = j;
                                    break;
                                }
                            }
                            if (currentRoomIndex != -1) {
                                break;
                            }
                        }

                        if (currentRoomIndex != -1) {
                            printf("Received message from client %d in room %s: %s\n", clientfd, rooms[currentRoomIndex].name, message);
                            broadcastMessageToRoom(message, clientfd, currentRoomIndex);
                        } else {
                            write(clientfd, "You are not currently in any chat room.", strlen("You are not currently in any chat room."));
                        }
                    }
                }
            }
        }
    }

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_ROOMS 10
#define MAX_NAME_LENGTH 100
#define MAX_PARTICIPANTS 100
#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct {
    char name[MAX_NAME_LENGTH];
    int participantCount;
    int participants[MAX_PARTICIPANTS];
} ChatRoom;

ChatRoom rooms[MAX_ROOMS];
int currentRoomCount = 0;

void createRoom(const char *roomName) {
    if (currentRoomCount >= MAX_ROOMS) {
        printf("Não é possível criar mais salas. Limite máximo atingido.\n");
        return;
    }

    strncpy(rooms[currentRoomCount].name, roomName, MAX_NAME_LENGTH - 1);
    rooms[currentRoomCount].name[MAX_NAME_LENGTH - 1] = '\0';
    rooms[currentRoomCount].participantCount = 0;
    currentRoomCount++;

    printf("Sala '%s' criada com sucesso.\n", roomName);
}

void listParticipants(int roomId) {
    if (roomId < 0 || roomId >= currentRoomCount) {
        printf("Sala inválida.\n");
        return;
    }

    ChatRoom *room = &rooms[roomId];

    printf("Participantes da sala '%s':\n", room->name);
    for (int i = 0; i < room->participantCount; i++) {
        printf("- Participante %d\n", room->participants[i]);
    }
}

void joinRoom(int roomId, int participantId) {
    if (roomId < 0 || roomId >= currentRoomCount) {
        printf("Sala inválida.\n");
        return;
    }

    ChatRoom *room = &rooms[roomId];

    if (room->participantCount >= MAX_PARTICIPANTS) {
        printf("Não é possível entrar na sala. Limite máximo de participantes atingido.\n");
        return;
    }

    room->participants[room->participantCount] = participantId;
    room->participantCount++;

    printf("Você entrou na sala '%s' como Participante %d.\n", room->name, participantId);
}

void handleClientCommand(int clientSocket, const char *command) {
    if (strncmp(command, "CREATE_ROOM", 11) == 0) {
        char roomName[MAX_NAME_LENGTH];
        sscanf(command, "CREATE_ROOM %[^\n]", roomName);
        createRoom(roomName);
        send(clientSocket, "Sala criada com sucesso.\n", 25, 0);
    } else if (strncmp(command, "LIST_PARTICIPANTS", 17) == 0) {
        int roomId;
        sscanf(command, "LIST_PARTICIPANTS %d", &roomId);
        listParticipants(roomId);
        send(clientSocket, "Lista de participantes enviada.\n", 32, 0);
    } else if (strncmp(command, "JOIN_ROOM", 9) == 0) {
        int roomId, participantId;
        sscanf(command, "JOIN_ROOM %d %d", &roomId, &participantId);
        joinRoom(roomId, participantId);
        send(clientSocket, "Entrou na sala com sucesso.\n", 28, 0);
    } else if (strncmp(command, "QUIT", 4) == 0) {
        send(clientSocket, "Encerrando conexão.\n", 21, 0);
        close(clientSocket);
    } else {
        send(clientSocket, "Comando inválido.\n", 18, 0);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso correto: %s <Porta_do_servidor>\n", argv[0]);
        exit(1);
    }

    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Falha ao criar socket");
        exit(1);
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(atoi(argv[1]));

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Falha ao fazer bind");
        exit(1);
    }

    if (listen(serverSocket, 5) < 0) {
        perror("Falha ao ouvir");
        exit(1);
    }

    printf("Servidor ouvindo na porta %s ...\n\n", argv[1]);

    // Variáveis para I/O multiplexado
    fd_set readSet;
    int maxFd;
    int clientSockets[MAX_CLIENTS];
    int clientCount = 0;

    // Inicializa o conjunto de descritores de arquivo
    FD_ZERO(&readSet);
    FD_SET(serverSocket, &readSet);
    maxFd = serverSocket;

    while (1) {
        fd_set tempReadSet = readSet;

        // Aguarda por atividade em algum socket
        if (select(maxFd + 1, &tempReadSet, NULL, NULL, NULL) < 0) {
            perror("Falha na chamada select");
            exit(1);
        }

        // Verifica se há atividade no socket do servidor
        if (FD_ISSET(serverSocket, &tempReadSet)) {
            // Aceita a conexão do cliente
            clientAddrLen = sizeof(clientAddr);
            clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
            if (clientSocket < 0) {
                perror("Falha ao aceitar conexão");
                exit(1);
            }

            printf("Cliente conectado: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

            // Adiciona o socket do cliente ao conjunto de descritores de arquivo
            FD_SET(clientSocket, &readSet);

            // Atualiza o valor máximo de descritor de arquivo
            if (clientSocket > maxFd) {
                maxFd = clientSocket;
            }

            // Armazena o socket do cliente na lista de sockets dos clientes
            clientSockets[clientCount] = clientSocket;
            clientCount++;

            // Envia a mensagem de boas-vindas ao cliente
            send(clientSocket, "Bem-vindo ao servidor de chat.\n", 30, 0);
        }

        // Verifica os sockets dos clientes para atividade de leitura
        for (int i = 0; i < clientCount; i++) {
            clientSocket = clientSockets[i];

            if (FD_ISSET(clientSocket, &tempReadSet)) {
                char buffer[MAX_BUFFER_SIZE];
                memset(buffer, 0, sizeof(buffer));

                // Lê a mensagem enviada pelo cliente
                ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead < 0) {
                    perror("Falha ao receber mensagem do cliente");
                    exit(1);
                } else if (bytesRead == 0) {
                    // O cliente encerrou a conexão
                    printf("Cliente desconectado: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

                    // Remove o socket do cliente do conjunto de descritores de arquivo
                    FD_CLR(clientSocket, &readSet);

                    // Fecha o socket do cliente
                    close(clientSocket);

                    // Remove o socket do cliente da lista de sockets dos clientes
                    for (int j = i; j < clientCount - 1; j++) {
                        clientSockets[j] = clientSockets[j + 1];
                    }
                    clientCount--;

                    continue;
                }

                printf("Mensagem recebida do cliente %s:%d: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buffer);

                // Trata o comando enviado pelo cliente
                handleClientCommand(clientSocket, buffer);
            }
        }
    }

    return 0;
}

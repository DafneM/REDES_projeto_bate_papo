#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <signal.h>

#include "cliente.h"

#define MAX_CLIENTS 10
#define MAX_MESSAGE_LENGTH 256
#define MAX_SALAS 10
// Global variables
clienteInfo cliente[MAX_CLIENTS];
salaInfo salas[MAX_SALAS];
// int clients[MAX_CLIENTS];
char messages[MAX_CLIENTS][MAX_MESSAGE_LENGTH];
fd_set readfds;
int maxfd;

void substitui_n(char *str)
{
    int len = strlen(str);
    if (len > 0)
    {
        for (int i = len - 1; i >= 0; i--)
        {
            if (str[i] == '\n')
            {
                str[i] = '\0';
                break;
            }
        }
    }
}

void createRoom(int sd, char *roomName)
{
    int countSalas = 0;
    Mensagem msg;
    msg.tipo = CRIAR_SALA;
    int nomeExiste = 0;

    for (int i = 0; i <= MAX_SALAS; i++)
    {   
        if (strcmp(salas[i].nome, roomName) == 0)
        {
            nomeExiste = 1;
        }
        else if (salas[i].id != -1)
        {
            countSalas++;
        }
    }

    if (countSalas == MAX_SALAS)
    {
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "Número máximo de salas esgotado, não foi possível criar mais salas!");
    }
    else if (nomeExiste == 1)
    {
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "Esta sala já existe, não foi possível criar mais salas!");
    } else {
        for (int i = 0; i <= MAX_SALAS; i++){
            if((salas[i].id == -1)){
                salas[i].id = i;
                strcpy(salas[i].nome, roomName);
                break;
            }
        }
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "Sala %s criada com sucesso!", roomName);
    }

    printf("antes do write\n");
    write(sd, &msg, sizeof(msg));
}

// Function to initialize the client array
void initClients()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        cliente[i].sd = -1;
    }
}

void initSalas()
{
    for (int i = 0; i < MAX_SALAS; i++)
    {
        salas[i].id = -1;
        strcpy(salas[i].nome, "");
    }
}
// Function to add a client to the chatroom
int addClient(int sd)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (cliente[i].sd == -1)
        {
            cliente[i].sd = sd;

            if (sd > maxfd)
            {
                maxfd = sd;
            }

            return 1;
        }
    }

    return 0;
}

// Function to remove a client from the chatroom
void removeClient(int clientfd)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (cliente[i].sd == clientfd)
        {
            cliente[i].sd = -1;
            break;
        }
    }
}

// Function to broadcast a message to all connected clients
void broadcastMessage(Mensagem *msg, int sender)
{
    char new_message[MAX_MESSAGE_LENGTH] = {0};
    if (strlen(msg->mensagem) == 0)
    {
        return;
    }
    snprintf(new_message, MAX_MESSAGE_LENGTH, "@%s: %s", msg->nome, msg->mensagem);
    strcpy(msg->mensagem, new_message);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        int clientfd = cliente[i].sd;

        if (clientfd != -1 && clientfd != sender)
        {
            write(clientfd, msg, sizeof(Mensagem));
        }
    }
}
void cadastrar_usuario(int sd, char *nome)
{
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (cliente[i].sd == sd)
        {

            getpeername(cliente[i].sd, (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
            strcpy(cliente[i].nome, nome);
            strcpy(cliente[i].ip, inet_ntoa(clientAddr.sin_addr));
            printf("Cliente @%s conectado no endereço %s\n", cliente[i].nome, cliente[i].ip);
            break;
        }
    }
    // listar_clientes();
}
void trataMensagem(int sd, Mensagem *msg)
{
    printf("Tratando mensagem...\n");
    printf("Tipo: %d\n", msg->tipo);
    switch (msg->tipo)
    {
    case CRIAR_SALA:
        createRoom(sd, msg->mensagem);
        break;
    case ENTRAR_SALA:
        break;
    case SAIR_SALA:
        break;
    case LISTAR_SALAS:
        listar_salas(sd);
        break;
    case LISTAR_USUARIOS:
        listar_usuarios(sd);
        break;
    case SAIR:
        break;
    case CADASTRAR_USUARIO:
        cadastrar_usuario(sd, msg->nome);
        break;
    default:
        broadcastMessage(msg, sd);
        break;
    }
}

void listar_usuarios(int cliente_sd)
{
    printf("Listando usuários...\n");
    Mensagem msg;
    memset(&msg, 0, sizeof(msg));
    msg.tipo = LISTAR_USUARIOS;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (cliente[i].sd != -1)
        {
            strcat(msg.mensagem, cliente[i].nome);
            strcat(msg.mensagem, "\n");
        }
    }
    substitui_n(msg.mensagem);

    write(cliente_sd, &msg, sizeof(msg));
}

void listar_salas(int cliente_sd)
{
    printf("Listando Salas...\n");
    Mensagem msg;
    memset(&msg, 0, sizeof(msg));
    msg.tipo = LISTAR_SALAS;
    for (int i = 0; i < MAX_SALAS; i++)
    {
        printf("Sala %d: %s\n", salas[i].id, salas[i].nome);
        if (salas[i].id != -1)
        {
            strcat(msg.mensagem, salas[i].nome);
            strcat(msg.mensagem, "\n");
        }
    }
    substitui_n(msg.mensagem);
    printf("Mensagem: %s\n", msg.mensagem);
    write(cliente_sd, &msg, sizeof(msg));
}

void trataCtrlC(int sig)
{
    printf("\nRecebido o sinal SIGINT (Ctrl+C)\n");
    exit(0);
}

int main()
{
    int serverfd, newclientfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int activity, i, valread;
    Mensagem msg;

    // Trata o sinal de ctrl+c
    signal(SIGINT, trataCtrlC);
    // Create a socket for the server
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    // Bind the socket to the specified address and port
    if (bind(serverfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverfd, MAX_CLIENTS) == -1)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    initClients();
    initSalas();
    maxfd = serverfd;

    printf("Chatroom server started. Waiting for connections...\n");

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(serverfd, &readfds);

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            int clientfd = cliente[i].sd;

            if (clientfd != -1)
            {
                FD_SET(clientfd, &readfds);
            }
        }

        activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity == -1)
        {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(serverfd, &readfds))
        {
            // New connection request
            newclientfd = accept(serverfd, (struct sockaddr *)&clientAddr, &addrLen);
            if (newclientfd == -1)
            {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            char teste[16];

            // Add the new client to the chatroom
            if (addClient(newclientfd) == 0)
            {
                printf("Chatroom is full. Connection rejected.\n");
                close(newclientfd);
            }
            else
            {
                printf("New client connected. Socket fd is %d\n", newclientfd);
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            int clientfd = cliente[i].sd;

            if (clientfd != -1 && FD_ISSET(clientfd, &readfds))
            {
                // Client activity
                FD_CLR(clientfd, &readfds);
                int valread = read(clientfd, &msg, sizeof(Mensagem));
                printf("%s\n", msg.mensagem);

                if (valread == 0)
                {
                    // Client disconnected
                    getpeername(clientfd, (struct sockaddr *)&clientAddr, &addrLen);
                    printf("Client disconnected. IP address: %s, Port: %d\n",
                           inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

                    // Remove the client from the chatroom
                    removeClient(clientfd);
                    close(clientfd);
                }
                else
                {
                    // Processa messagem
                    trataMensagem(clientfd, &msg);
                }
            }
        }
    }

    return 0;
}
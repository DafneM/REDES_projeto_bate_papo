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
clienteInfo clientes[MAX_CLIENTS];
salaInfo salas[MAX_SALAS];
char messages[MAX_CLIENTS][MAX_MESSAGE_LENGTH];
fd_set readfds;
int maxfd;

void replaceN(char *str)
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
    printf("Criando sala %s\n", roomName);
    for (int i = 0; i < MAX_SALAS; i++)
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
    }
    else
    {
        for (int i = 0; i < MAX_SALAS; i++)
        {
            if ((salas[i].id == -1))
            {
                salas[i].id = i;
                strcpy(salas[i].nome, roomName);
                break;
            }
        }
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "Sala %s criada com sucesso!", roomName);
    }

    write(sd, &msg, sizeof(msg));
}

int deleteRoom(int sd, char *roomName)
{
    Mensagem msg;
    msg.tipo = DELETAR_SALA;
    int roomFound = 0;
    int roomEmpty = 0;
    printf("Excluindo sala %s\n", roomName);
    for (int i = 0; i < MAX_SALAS; i++)
    {

        if (strcmp(salas[i].nome, roomName) == 0)
        {
            roomFound = 1;
            salas[i].id = -1;
            salas[i].nome[0] = '\0';
            if (salas[i].qtdClientes == 0)
                roomEmpty = 1;
            break;
        }
    }

    if (!roomEmpty)
    {
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "Sala \"%s\" não pode ser excluída pois ainda há clientes nela.", roomName);
    }
    else if (!roomFound)
    {
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "Sala \"%s\" não encontrada, não é possível excluí-la.", roomName);
    }
    else
    {
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "Sala \"%s\" excluída com sucesso!", roomName);
    }

    write(sd, &msg, sizeof(msg));
    return 0;
}
void joinRoom(clienteInfo *cliente, char *roomName)
{
    int countSalas = 0;
    Mensagem msg;
    msg.tipo = ENTRAR_SALA;
    int indiceSala = -1, indiceCliente = -1;
    printf("@%s entrando na sala %s\n", cliente->nome, roomName);
    for (int i = 0; i < MAX_SALAS; i++)
    {
        if (strcmp(salas[i].nome, roomName) == 0)
        {
            indiceSala = i;
            break;
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clientes[i].nome == cliente->nome)
        {
            indiceCliente = i;
            break;
        }
    }

    if (indiceSala != -1 && salas[indiceSala].qtdClientes < MAX_CLIENTS)
    {
        salas[indiceSala].qtdClientes++;
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "Você entrou na sala %s!", roomName);
        cliente->idSala = salas[indiceSala].id;
    }
    else if (indiceSala != -1 && salas[indiceSala].qtdClientes == MAX_CLIENTS)
    {
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "A sala %s está cheia!", roomName);
    }
    else
    {
        snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "A sala %s não existe!", roomName);
    }
    write(cliente->sd, &msg, sizeof(msg));
}

int leaveRoom(clienteInfo *cliente, char *roomName)
{
    Mensagem msg;
    memset(&msg, 0, sizeof(Mensagem));
    int indiceCliente;
    int indiceSala = clientes[indiceCliente].idSala;

    printf("@%s saindo da sala %s\n", cliente->nome, roomName);

    msg.tipo = SAIR_SALA;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clientes[i].idSala == indiceSala && indiceSala != -1 && clientes[i].sd == cliente->sd && strcmp(clientes[i].nome, cliente->nome) == 0)
        {
            clientes[i].idSala = -1;
            cliente->idSala = clientes[i].idSala;
            break;
        }
    }
    snprintf(msg.mensagem, MAX_MESSAGE_LENGTH, "Você saiu da sala!");
    write(cliente->sd, &msg, sizeof(msg));
    return 1;
}

void initClients()
{

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clientes[i].sd = -1;
        clientes[i].idSala = -1;
        strcpy(clientes[i].nome, "");
        strcpy(clientes[i].ip, "");
    }
}

void initSalas()
{
    for (int i = 0; i < MAX_SALAS; i++)
    {
        salas[i].id = -1;
        salas[i].qtdClientes = 0;
        strcpy(salas[i].nome, "");
    }
}

int addClient(int sd)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clientes[i].sd == -1)
        {
            clientes[i].sd = sd;

            if (sd > maxfd)
            {
                maxfd = sd;
            }
            return 1;
        }
    }

    return 0;
}

void removeClient(int clientfd)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clientes[i].sd == clientfd)
        {
            salas[clientes[i].idSala].qtdClientes--;
            clientes[i].idSala = -1;
            clientes[i].sd = -1;
            strcpy(clientes[i].nome, "");
            clientes[i].ip[0] = '\0';
            break;
        }
    }
}

void broadcastMessage(Mensagem *msg, clienteInfo *sender)
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
        int clientfd = clientes[i].sd;

        if (clientfd != -1 && clientfd != sender->sd && clientes[i].idSala == sender->idSala)
        {
            write(clientfd, msg, sizeof(Mensagem));
        }
    }
}
void registerUser(int sd, char *nome)
{
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clientes[i].sd == sd)
        {

            getpeername(clientes[i].sd, (struct sockaddr *)&clientAddr, (socklen_t *)&addrLen);
            strcpy(clientes[i].nome, nome);
            strcpy(clientes[i].ip, inet_ntoa(clientAddr.sin_addr));
            printf("Cliente @%s conectado no endereço %s\n", clientes[i].nome, clientes[i].ip);
            break;
        }
    }
}

void listUsers(clienteInfo *cliente)
{
    printf("Listando usuários...\n");
    Mensagem msg;
    memset(&msg, 0, sizeof(msg));
    msg.tipo = LISTAR_USUARIOS;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (cliente->sd != -1 && clientes[i].idSala == cliente->idSala && clientes[i].idSala != -1)
        {
            strcat(msg.mensagem, clientes[i].nome);
            strcat(msg.mensagem, "\n");
        }
    }
    replaceN(msg.mensagem);

    write(cliente->sd, &msg, sizeof(msg));
}

void processMessages(clienteInfo *cliente, Mensagem *msg)
{
    switch (msg->tipo)
    {
    case CRIAR_SALA:
        createRoom(cliente->sd, msg->mensagem);
        break;
    case ENTRAR_SALA:
        joinRoom(cliente, msg->mensagem);
        break;
    case DELETAR_SALA:
        if (deleteRoom(cliente->sd, msg->mensagem) == 1)
        {
            return;
        }
        break;
    case SAIR_SALA:
        if (leaveRoom(cliente, msg->mensagem) == 1)
        {
            return;
        }
        break;
    case LISTAR_SALAS:
        listRooms(cliente->sd);
        break;
    case LISTAR_USUARIOS:
        listUsers(cliente);
        break;
    case SAIR:
        break;
    case CADASTRAR_USUARIO:
        registerUser(cliente->sd, msg->nome);
        break;
    default:
        broadcastMessage(msg, cliente);
        break;
    }
}

void listRooms(int cliente_sd)
{
    printf("Listando Salas...\n");
    Mensagem msg;
    memset(&msg, 0, sizeof(msg));
    msg.tipo = LISTAR_SALAS;
    for (int i = 0; i < MAX_SALAS; i++)
    {
        if (salas[i].id != -1)
        {
            strcat(msg.mensagem, salas[i].nome);
            strcat(msg.mensagem, "\n");
        }
    }
    replaceN(msg.mensagem);
    write(cliente_sd, &msg, sizeof(msg));
}

void handleCtrlC(int sig)
{
    printf("\nFinalizando Servidor(Ctrl+C)\n");
    exit(0);
}

int main()
{
    int serverfd, newclientfd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int activity, i, valread;
    Mensagem msg;

    signal(SIGINT, handleCtrlC);
    serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverfd, MAX_CLIENTS) == -1)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    initClients();
    initSalas();
    maxfd = serverfd;

    printf("Servidor de sala de bate-papo iniciado. Aguardando conexões...\n");

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(serverfd, &readfds);

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            int clientfd = clientes[i].sd;

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
            newclientfd = accept(serverfd, (struct sockaddr *)&clientAddr, &addrLen);
            if (newclientfd == -1)
            {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            char teste[16];

            if (addClient(newclientfd) == 0)
            {
                printf("A sala de bate-papo está cheia. Conexão rejeitada.\n");
                close(newclientfd);
            }
            else
            {
                printf("Novo cliente conectado. O descritor de socket é %d\n", newclientfd);
            }
        }

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            int clientfd = clientes[i].sd;

            if (clientfd != -1 && FD_ISSET(clientfd, &readfds))
            {
                FD_CLR(clientfd, &readfds);
                int valread = read(clientfd, &msg, sizeof(Mensagem));

                if (valread == 0)
                {
                    getpeername(clientfd, (struct sockaddr *)&clientAddr, &addrLen);
                    printf("Cliente desconectado. Endereço IP: %s, Porta: %d\n",
                           inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                    removeClient(clientfd);
                    close(clientfd);
                }
                else
                {
                    processMessages(&clientes[i], &msg);
                }
            }
        }
    }

    return 0;
}
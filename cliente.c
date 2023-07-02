#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <signal.h>

#include "cliente.h"

#define MAX_SIZE 255
int returnToMenu = 0;
int sdCliente = -1;
int flagSala = 0;

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

void limpa_cmd()
{
    printf("\r%s", "> ");
    fflush(stdout);
}

void trataCtrlC(int sig)
{
    Mensagem msg;
    int n = -1;
    msg.tipo = SAIR_SALA;
    if (sdCliente != -1)
    {
        if (flagSala)
        {
            printf("\nSaindo da sala...\n");
            send(sdCliente, &msg, sizeof(Mensagem), 0); /* enviando dados ...  */
            n = recv(sdCliente, &msg, sizeof(Mensagem), 0);
        }
        if (n > 0)
        {
            printf("\r%s\n", msg.mensagem);
            limpa_cmd();
        }
    }
    close(sdCliente);
    printf("\nFinalizando conexão com o servidor(Ctrl+C)\n");
    exit(0);
}
void createRoom(Mensagem *msg, clienteInfo *cliente)
{
    char roomName[30];

    printf("Digite o nome da sala que deseja criar: ");
    scanf("%s", roomName);

    strcpy(msg->mensagem, roomName);
    replaceN(msg->mensagem);
}

void joinRoom(Mensagem *msg)
{
    char roomName[30];

    printf("Digite o nome da sala: ");
    scanf("%s", roomName);

    strcpy(msg->mensagem, roomName);
    replaceN(msg->mensagem);
}

void deleteRoom(Mensagem *msg)
{
    char roomName[30];

    printf("Digite o nome da sala que você deseja deletar: ");
    scanf("%s", roomName);

    strcpy(msg->mensagem, roomName);
    replaceN(msg->mensagem);
}

void helpMenu()
{
    printf("\n----- Ajuda -------\n");
    printf("Comandos fora da sala:\n");
    printf("/c - Criar sala\n");
    printf("/d - Deletar sala\n");
    printf("/e - Entrar em uma sala\n");
    printf("/l - Listar salas\n");
    printf("/SAIR - Fechar\n");
    printf("\nComandos dentro da sala:\n");
    printf("/l - Listar salas\n");
    printf("/u - Listar usuários na sua sala\n");
    printf("/h - Ajuda\n");
    printf("/s - Sair da sala\n");
    printf("---------------------\n");
}

void showComands(clienteInfo *cliente)
{
    char option[5];
    Mensagem msg;
    memset(&msg, 0, sizeof(Mensagem));

    strcpy(msg.nome, cliente->nome);
    while (1)
    {
        printf("\nEscolha uma opção:\n");
        printf("/c - Criar sala\n");
        printf("/d - Deletar sala\n");
        printf("/e - Entrar em uma sala\n");
        printf("/l - Listar salas\n");
        printf("/h - Ajuda\n");
        printf("/SAIR - Fechar\n");
        printf("Opção: ");
        scanf("%s", option);
        printf("\n");
        if (strncmp(option, "/c", 2) == 0)
        {
            msg.tipo = CRIAR_SALA;
            createRoom(&msg, cliente);
            printf("Cria sala: %sn", msg.mensagem);
            send(cliente->sd, &msg, sizeof(Mensagem), 0); /* enviando dados ...  */
            recebe_mensagem(cliente->sd, cliente);
        }
        else if (strncmp(option, "/e", 2) == 0)
        {
            msg.tipo = ENTRAR_SALA;
            joinRoom(&msg);
            send(cliente->sd, &msg, sizeof(Mensagem), 0); /* enviando dados ...  */
            recebe_mensagem(cliente->sd, cliente);
            flagSala = 1;
            break;
        }
        else if (strncmp(option, "/d", 2) == 0)
        {
            msg.tipo = DELETAR_SALA;
            deleteRoom(&msg);
            send(cliente->sd, &msg, sizeof(Mensagem), 0); /* enviando dados ...  */
            recebe_mensagem(cliente->sd, cliente);
        }
        else if (strncmp(option, "/l", 2) == 0)
        {
            msg.tipo = LISTAR_SALAS;
            send(cliente->sd, &msg, sizeof(Mensagem), 0); /* enviando dados ...  */
            recebe_mensagem(cliente->sd, cliente);
        }
        else if (strncmp(option, "/SAIR", 2) == 0)
        {
            trataCtrlC(SIGINT);
            break;
        }
        else if (strncmp(option, "/h", 2) == 0)
        {
            helpMenu();
        }
        else
        {
            printf("Opção inválida.\n");
        }

        if (returnToMenu && cliente->idSala != -1)
        {
            returnToMenu = 0;
            break;
        }
    }
}

void processSendMessages(Mensagem *msg, char *str, clienteInfo *cliente)
{

    strcpy(msg->nome, cliente->nome);

    if (strncmp(str, "/", 1) == 0)
    {
        switch (str[1])
        {
        case 'c':
            printf("Não é possível criar sala dentro de uma sala. Volte ao menu!\n");
            break;
        case 'e':
            printf("Não é possível entrar em uma sala dentro de uma sala. Volte ao menu!\n");
            break;
        case 'd':
            printf("Não é possível deletar sala dentro de uma sala. Volte ao menu!\n");
            break;
        case 's':
            msg->tipo = SAIR_SALA;
            flagSala = 0;
            break;
        case 'l':
            msg->tipo = LISTAR_SALAS;
            break;
        case 'u':
            msg->tipo = LISTAR_USUARIOS;
            break;
        case 'h':
            helpMenu();
            break;
        default:
            printf("Comando inválido.\n");
            break;
        }
    }
    else
    {
        strcpy(msg->mensagem, str);
        replaceN(msg->mensagem);
    }
}

void processReceiveMessages(Mensagem *msg, clienteInfo *cliente)
{
    switch (msg->tipo)
    {
    case CRIAR_SALA:
        msg->tipo = CRIAR_SALA;
        break;
    case ENTRAR_SALA:
        msg->tipo = ENTRAR_SALA;
        break;
    case DELETAR_SALA:
        msg->tipo = DELETAR_SALA;
        break;
    case SAIR_SALA:
        msg->tipo = SAIR_SALA;
        cliente->idSala = -1;
        returnToMenu = 1;
        break;
    case LISTAR_SALAS:
        printf("----- LISTA DE SALAS ----\n");
        break;
    case LISTAR_USUARIOS:
        printf("----- LISTA DE USUÁRIOS ----\n");
        break;
    default:
        break;
    }
    limpa_cmd();
}

void sendMessages(int sd, clienteInfo *cliente)
{
    Mensagem msg;
    char str[MAX_SIZE];
    memset(&msg, 0, sizeof(Mensagem));
    memset(&str, 0, sizeof(str));

    limpa_cmd();
    fgets(str, MAX_SIZE, stdin) != NULL;
    processSendMessages(&msg, str, cliente);

    if (strlen(msg.mensagem) == 0)
    {
        limpa_cmd();
    }
    send(sd, &msg, sizeof(Mensagem), 0); /* enviando dados ...  */
    if (strcmp(msg.mensagem, "FIM") == 0)
    {
        printf("Saindo...\n");
    }
}

void recebe_mensagem(int sd, clienteInfo *cliente)
{
    Mensagem bufin;
    memset(&bufin, 0, sizeof(bufin));
    int n;
    limpa_cmd();
    n = recv(sd, &bufin, sizeof(Mensagem), 0);
    if (n > 0)
    {
        processReceiveMessages(&bufin, cliente);
        printf("\r%s\n", bufin.mensagem);
        limpa_cmd();
    }
}

int main(int argc, char *argv[])
{
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    char nome_usuario[50];
    clienteInfo cliente;
    fd_set readfds;
    Mensagem msg;
    signal(SIGINT, trataCtrlC);

    if (argc < 3)
    {
        printf("uso correto: %s <ip_do_servidor> <porta_do_servidor>\n", argv[0]);
        exit(1);
    }

    printf("Digite seu nome de usuário: ");
    if (fgets(cliente.nome, 50, stdin) != NULL)
        replaceN(cliente.nome);

    /* configura endereco do servidor */
    serverAddr.sin_family = AF_INET; /* config. socket p. internet*/
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddr.sin_port = htons(atoi(argv[2]));

    cliente.sd = socket(AF_INET, SOCK_STREAM, 0);
    sdCliente = cliente.sd;
    if (cliente.sd < 0)
    {
        fprintf(stderr, "Criacao do socket falhou!\n");
        exit(1);
    }

    /* Conecta socket ao servidor definido */
    if (connect(cliente.sd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        fprintf(stderr, "Tentativa de conexao falhou!\n");
        exit(1);
    }

    if (getpeername(cliente.sd, (struct sockaddr *)&clientAddr, &addrLen) == -1)
    {
        perror("Erro ao obter o endereço IP do cliente");
        exit(EXIT_FAILURE);
    }

    strncpy(cliente.ip, inet_ntoa(clientAddr.sin_addr), sizeof(cliente.ip) - 1);
    printf("Conectado ao servidor %s:%d\n", cliente.ip, ntohs(clientAddr.sin_port));
    printf("Bem vindo %s!\n", cliente.nome);

    msg.tipo = CADASTRAR_USUARIO;
    strcpy(msg.nome, cliente.nome);
    send(cliente.sd, &msg, sizeof(Mensagem), 0);
    cliente.idSala = -1;

    showComands(&cliente);
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(cliente.sd, &readfds);
        limpa_cmd();

        if (select(cliente.sd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("Select falhou!");
            exit(1);
        }
        else if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            sendMessages(cliente.sd, &cliente);
        }
        else if (FD_ISSET(cliente.sd, &readfds))
        {
            recebe_mensagem(cliente.sd, &cliente);
            if (returnToMenu == 1)
            {
                showComands(&cliente);
            }
        }
    }
    close(cliente.sd);
    return 0;
}
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

void limpa_cmd()
{
    printf("\r%s", "> ");
    fflush(stdout);
}

void trataCtrlC(int sig)
{
    printf("\nRecebido o sinal SIGINT (Ctrl+C)\n");
    exit(0);
}
void createRoom(Mensagem *msg, clienteInfo *cliente)
{
    char roomName[30];

    printf("Digite o nome da sala que deseja criar: ");
    scanf("%s", roomName);

    strcpy(msg->mensagem, roomName);
    substitui_n(msg->mensagem);

    // envia_mensagem(msg, cliente);
    // recebe_mensagem(cliente->sd);
}

void joinRoom(Mensagem *msg)
{
    char roomName[30];

    printf("Digite o nome da sala: ");
    scanf("%s", roomName);

    strcpy(msg->mensagem, roomName);
    substitui_n(msg->mensagem);
}

void mostra_comandos(clienteInfo *cliente)
{
    char option[5];
    Mensagem msg;
    memset(&msg, 0, sizeof(Mensagem));

    strcpy(msg.nome, cliente->nome);
    while (1)
    {
        printf("\nEscolha uma opção:\n");
        printf("/c - Criar sala\n");
        printf("/e - Entrar em uma sala\n");
        printf("/l - Listar salas\n");
        printf("/s - Sair\n");
        printf("Opção: ");
        scanf("%s", option);
        printf("\n");
        printf("Opção escolhida: %s\n", option);
        if (strncmp(option, "/c", 2) == 0)
        {
            msg.tipo = CRIAR_SALA;
            createRoom(&msg, cliente);
            printf("Cria sala: %sn", msg.mensagem);
            send(cliente->sd, &msg, sizeof(Mensagem), 0); /* enviando dados ...  */
            recebe_mensagem(cliente->sd);
        }
        else if (strncmp(option, "/e", 2) == 0)
        {
            msg.tipo = ENTRAR_SALA;
            joinRoom(&msg);
            send(cliente->sd, &msg, sizeof(Mensagem), 0); /* enviando dados ...  */
            recebe_mensagem(cliente->sd);
            break;
        }
        else if (strncmp(option, "/l", 2) == 0)
        {
            msg.tipo = LISTAR_SALAS;
            send(cliente->sd, &msg, sizeof(Mensagem), 0); /* enviando dados ...  */
            recebe_mensagem(cliente->sd);
        }
        else if (strncmp(option, "/s", 2) == 0)
        {
            msg.tipo = SAIR_SALA;
            break;
        }
        else
        {
            printf("Opção inválida.\n");
        }
    }
}

void trata_envia_mensagem(Mensagem *msg, char *str, clienteInfo *cliente)
{

    strcpy(msg->nome, cliente->nome);

    if (strncmp(str, "/", 1) == 0)
    {
        switch (str[1])
        {
        case 'c':
            msg->tipo = CRIAR_SALA;
            createRoom(msg, cliente);
            break;
        case 'e':
            msg->tipo = ENTRAR_SALA;
            break;
        case 's':
            msg->tipo = SAIR_SALA;
            break;
        case 'l':
            msg->tipo = LISTAR_SALAS;
            break;
        case 'u':
            msg->tipo = LISTAR_USUARIOS;
            break;
        case 'h':
            break;
        default:
            printf("Comando inválido.\n");
            break;
        }
    }
    else
    {
        strcpy(msg->mensagem, str);
        substitui_n(msg->mensagem);
    }
}

void trata_recebe_mensagem(Mensagem *msg)
{
    switch (msg->tipo)
    {
    case CRIAR_SALA:
        msg->tipo = CRIAR_SALA;
        break;
    case ENTRAR_SALA:
        msg->tipo = ENTRAR_SALA;
        break;
    case SAIR_SALA:
        msg->tipo = SAIR_SALA;
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

void envia_mensagem(int sd, clienteInfo *cliente)
{
    Mensagem msg;
    char str[MAX_SIZE];
    memset(&msg, 0, sizeof(Mensagem));
    memset(&str, 0, sizeof(str));

    limpa_cmd();
    fgets(str, MAX_SIZE, stdin) != NULL;
    trata_envia_mensagem(&msg, str, cliente);

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

void recebe_mensagem(int sd)
{
    Mensagem bufin;
    memset(&bufin, 0, sizeof(bufin));
    int n;
    limpa_cmd();
    n = recv(sd, &bufin, sizeof(Mensagem), 0);
    if (n > 0)
    {
        trata_recebe_mensagem(&bufin);
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

    // Trata o sinal de ctrl+c
    signal(SIGINT, trataCtrlC);

    if (argc < 3)
    {
        printf("uso correto: %s <ip_do_servidor> <porta_do_servidor>\n", argv[0]);
        exit(1);
    }

    printf("Digite seu nome de usuário: ");
    if (fgets(cliente.nome, 50, stdin) != NULL)
        substitui_n(cliente.nome);

    /* configura endereco do servidor */
    serverAddr.sin_family = AF_INET; /* config. socket p. internet*/
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddr.sin_port = htons(atoi(argv[2]));

    cliente.sd = socket(AF_INET, SOCK_STREAM, 0);
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

    // Copie o endereço IP para cliente.ip
    strncpy(cliente.ip, inet_ntoa(clientAddr.sin_addr), sizeof(cliente.ip) - 1);
    printf("Conectado ao servidor %s:%d\n", cliente.ip, ntohs(clientAddr.sin_port));
    printf("Bem vindo %s!\n", cliente.nome);

    msg.tipo = CADASTRAR_USUARIO;
    strcpy(msg.nome, cliente.nome);
    send(cliente.sd, &msg, sizeof(Mensagem), 0);

    mostra_comandos(&cliente);
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
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            envia_mensagem(cliente.sd, &cliente);
        }
        if (FD_ISSET(cliente.sd, &readfds))
        {
            recebe_mensagem(cliente.sd);
        }
    }
    close(cliente.sd);
    return 0;
}
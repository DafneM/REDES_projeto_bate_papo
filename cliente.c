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
    char *newline_pos = strchr(str, '\n');
    if (newline_pos != NULL)
    {
        *newline_pos = '\0';
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

void trata_envia_mensagem(Mensagem *msg, clienteInfo *cliente)
{

    char str[MAX_SIZE];
    memset(&str, 0, sizeof(str));
    strcpy(msg->nome, cliente->nome);

    limpa_cmd();
    fgets(str, MAX_SIZE, stdin) != NULL;
    if (strncmp(str, "/c", 2) == 0)
    {
        switch (str[2] - '0')
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
            msg->tipo = LISTAR_SALAS;
            break;
        case LISTAR_USUARIOS:
            msg->tipo = LISTAR_USUARIOS;
            break;
        default:
            break;
        }
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
        msg->tipo = LISTAR_SALAS;
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
    memset(&msg, 0, sizeof(Mensagem));

    trata_envia_mensagem(&msg, cliente);

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
    n = recv(sd, &bufin, sizeof(bufin), 0);
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

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(cliente.sd, &readfds);
        fflush(stdin);

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
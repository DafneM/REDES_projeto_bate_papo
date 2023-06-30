#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>

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

void envia_mensagem(int sd, clienteInfo cliente)
{
    char bufout[MAX_SIZE];
    memset(bufout, 0, sizeof(bufout));

    limpa_cmd();
    fgets(bufout, MAX_SIZE, stdin) != NULL;

    substitui_n(bufout);
    if (strlen(bufout) == 0)
    {
        limpa_cmd();
    }

    send(sd, bufout, MAX_SIZE, 0); /* enviando dados ...  */
    if (strcmp(bufout, "FIM") == 0)
    {
        printf("Saindo...\n");
    }
}

void recebe_mensagem(int sd)
{
    char bufin[MAX_SIZE];
    int n;

    memset(bufin, 0, sizeof(bufin));

    n = recv(sd, bufin, sizeof(bufin), 0);
    if (n > 0)
    {
        printf("\r%s\n", bufin);
        limpa_cmd();
    }
}

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    char nome_usuario[50];
    clienteInfo clienteInfo;
    fd_set readfds;

    if (argc < 3)
    {
        printf("uso correto: %s <ip_do_servidor> <porta_do_servidor>\n", argv[0]);
        exit(1);
    }

    // printf("Digite seu nome de usuário: ");
    // if (fgets(clienteInfo.nome, 50, stdin) != NULL)
    //     substitui_n(clienteInfo.nome);

    /* configura endereco do servidor */
    serverAddr.sin_family = AF_INET; /* config. socket p. internet*/
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddr.sin_port = htons(atoi(argv[2]));

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        fprintf(stderr, "Criacao do socket falhou!\n");
        exit(1);
    }

    /* Conecta socket ao servidor definido */
    if (connect(sd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        fprintf(stderr, "Tentativa de conexao falhou!\n");
        exit(1);
    }

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sd, &readfds);
        fflush(stdin);

        if (select(sd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("Select falhou!");
            exit(1);
        }
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            envia_mensagem(sd, clienteInfo);
        }
        if (FD_ISSET(sd, &readfds))
        {
            recebe_mensagem(sd);
        }
    }
    close(sd);
    return 0;
}
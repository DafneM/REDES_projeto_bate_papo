#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>

#define QLEN       5          /* tamanho da fila de clientes  */
#define MAX_SIZE   80         /* tamanho do buffer */
#define MAX_ROOMS  10         /* número máximo de salas */
#define MAX_USERS  100        /* número máximo de usuários */

struct Room {
    char name[100];
    int limit;
    int participants[MAX_USERS];
    int numParticipants;
};

void menu() {
    int opcao, limit_users, id_sala;
    char nome[1000];

    printf("Olá, digite o que você deseja fazer\n");
    printf("1 - Criar sala virtual\n");
    printf("2 - Listar participantes de uma sala\n");
    printf("3 - Entrar em uma sala\n");
    printf("4 - Criar sala\n");

    scanf("%d", &opcao);
    if (opcao == 1) {
        printf("Digite o nome que deseja para a sala\n");
        scanf("%s", nome);
        printf("Agora digite o limite de participantes para a sala\n");
        scanf("%d", &limit_users);
        // Código para criar a sala com o nome e limite especificados
    } else if (opcao == 2) {
        printf("Digite o identificador da sala para listar os participantes\n");
        scanf("%d", &id_sala);
        // Código para listar os participantes da sala com o identificador especificado
    } else if (opcao == 3) {
        printf("Digite o identificador da sala para entrar\n");
        scanf("%d", &id_sala);
        // Código para entrar na sala com o identificador especificado
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in endServ;   /* endereco do servidor   */
    struct sockaddr_in endCli;    /* endereco do cliente    */
    int sd, novo_sd;              /* socket descriptors */
    int pid, alen, n;

    fd_set sockets_atuais, aux_sockets_atuais; //select é destrutivo e vai mudar os sockets, precisamos de uma cópia
    int max_sd;

    struct Room rooms[MAX_ROOMS]; // Array de salas
    int numRooms = 0;             // Contador de salas

    // Inicializar o conjunto de sockets
    FD_ZERO(&sockets_atuais); // Zerar conjunto de sockets atuais

    if (argc < 3) {
        printf("Digite IP e Porta para este servidor\n");
        exit(1);
    }

    memset((char *)&endServ, 0, sizeof(endServ));
    endServ.sin_family = AF_INET;              /* familia TCP/IP   */
    endServ.sin_addr.s_addr = inet_addr(argv[1]);  /* endereco IP      */
    endServ.sin_port = htons(atoi(argv[2])); /* PORTA    */

    /* Cria socket */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        fprintf(stderr, "Falha ao criar socket!\n");
        exit(1);
    }

    /* liga socket a porta e IP */
    if (bind(sd, (struct sockaddr *)&endServ, sizeof(endServ)) < 0) {
        fprintf(stderr, "Ligacao Falhou!\n");
        exit(1);
    }

    /* Ouve porta */
    if (listen(sd, QLEN) < 0) {
        fprintf(stderr, "Falhou ouvindo porta!\n");
        exit(1);
    }

    printf("Servidor ouvindo no IP %s, na porta %s ...\n\n", argv[1], argv[2]);

    /* Aceita conexoes */
    alen = sizeof(endCli);
    max_sd = sd; // Definir o valor inicial de max_sd como sd

    FD_SET(sd, &sockets_atuais);

    for (;;) {
        aux_sockets_atuais = sockets_atuais;

        if (select(max_sd + 1, &aux_sockets_atuais, NULL, NULL, NULL) < 0) {
            perror("Erro no select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &aux_sockets_atuais)) {
                if (i == sd) {
                    novo_sd = accept(sd, (struct sockaddr *)&endCli, &alen);
                    FD_SET(novo_sd, &sockets_atuais);

                    if (novo_sd > max_sd) {
                        max_sd = novo_sd;
                    }

                    printf("Cliente %s: %u conectado.\n", inet_ntoa(endCli.sin_addr), ntohs(endCli.sin_port));
                    atende_cliente(novo_sd, endCli, rooms, &numRooms);
                } else {
                    atende_cliente(i, endCli, rooms, &numRooms);
                    FD_CLR(i, &sockets_atuais);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

int atende_cliente(int descritor, struct sockaddr_in endCli, struct Room *rooms, int *numRooms) {
    char bufin[MAX_SIZE];
    int n;

    while (1) {
        memset(&bufin, 0x0, sizeof(bufin));
        n = recv(descritor, &bufin, sizeof(bufin), 0);

        if (n <= 0) {
            break;
        }

        if (strncmp(bufin, "FIM", 3) == 0) {
            break;
        }

        printf("[%s:%u] => %s\n", inet_ntoa(endCli.sin_addr), ntohs(endCli.sin_port), bufin);

        // Processar o comando recebido do cliente
        if (strncmp(bufin, "MENU", 4) == 0) {
            menu();
        }
    }

    printf("Encerrando conexao com %s:%u ...\n\n", inet_ntoa(endCli.sin_addr), ntohs(endCli.sin_port));
    close(descritor);

    return 0;
}

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

void sendCommand(int sd, const char *command) {
    if (send(sd, command, strlen(command), 0) < 0) {
        fprintf(stderr, "Erro ao enviar comando para o servidor\n");
        exit(1);
    }
}

void handleChatMessage(const char *message) {
    printf("Mensagem do chat: %s\n", message);
}

void handleServerResponse(int sd) {
    char buffer[MAX_SIZE];
    memset(buffer, 0, sizeof(buffer));

    if (recv(sd, buffer, sizeof(buffer), 0) < 0) {
        fprintf(stderr, "Erro ao receber resposta do servidor\n");
        exit(1);
    }

    if (strncmp(buffer, "MENSAGEM_CHAT", 13) == 0) {
        handleChatMessage(buffer);
    } else {
        printf("%s\n", buffer);
    }
}

void createRoom(int sd) {
    char roomName[MAX_NAME_LENGTH];

    printf("Digite o nome da sala: ");
    scanf("%s", roomName);

    char command[MAX_SIZE];
    snprintf(command, sizeof(command), "CREATE_ROOM %s", roomName);
    sendCommand(sd, command);

    handleServerResponse(sd);
}

void listParticipants(int sd) {
    char roomName[MAX_NAME_LENGTH];

    printf("Digite o nome da sala: ");
    scanf("%s", roomName);

    char command[MAX_SIZE];
    snprintf(command, sizeof(command), "LIST_PARTICIPANTS %s", roomName);
    sendCommand(sd, command);

    handleServerResponse(sd);
}

void joinRoom(int sd, clienteInfo *cliente) {
    char roomName[MAX_NAME_LENGTH];
    char participantName[MAX_NAME_LENGTH];

    printf("Digite o nome da sala: ");
    scanf("%s", roomName);

    printf("Digite o seu nome de participante: ");
    scanf("%s", participantName);

    char command[MAX_SIZE];
    snprintf(command, sizeof(command), "JOIN_ROOM %s %s", roomName, participantName);
    sendCommand(sd, command);

    handleServerResponse(sd);

    if (strncmp(command, "Entrou na sala com sucesso.", 28) == 0) {
        sscanf(command, "Entrou na sala com sucesso. Sala %d, Participante %d", &(cliente->roomId), &(cliente->participantId));
    } else {
        cliente->roomId = -1;
        cliente->participantId = -1;
    }
}

void chat(int sd, clienteInfo cliente) {
    int maxfd = sd + 1;
    fd_set readfds;
    char message[MAX_SIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);  // Adiciona o stdin (entrada do usuário) ao conjunto de descritores de arquivo
        FD_SET(sd, &readfds); // Adiciona o socket do servidor ao conjunto de descritores de arquivo

        // Monitora os descritores de arquivo para ver se há alguma atividade
        if (select(maxfd, &readfds, NULL, NULL, NULL) == -1) {
            perror("Erro ao executar select");
            exit(1);
        }

        // Verifica se há mensagens do servidor
        if (FD_ISSET(sd, &readfds)) {
            handleServerResponse(sd);
        }

        // Verifica se há mensagens do usuário
        if (FD_ISSET(0, &readfds)) {
            fgets(message, sizeof(message), stdin);
            substitui_n(message);
            envia_mensagem(sd, message);
        }
    }
}

void substitui_n(char *str) {
    int length = strlen(str);
    if (length > 0 && str[length - 1] == '\n') {
        str[length - 1] = '\0';
    }
}

void envia_mensagem(int sd, const char *message) {
    char command[MAX_SIZE];
    snprintf(command, sizeof(command), "SEND_MESSAGE %s", message);
    sendCommand(sd, command);
}

void showMenu(int sd) {
    int option;
    clienteInfo cliente;
    cliente.roomId = -1; // Inicializa o ID da sala como inválido

    while (1) {
        printf("\nEscolha uma opção:\n");
        printf("1 - Criar sala\n");
        printf("2 - Listar participantes de uma sala\n");
        printf("3 - Entrar em uma sala\n");
        printf("0 - Sair\n");
        printf("Opção: ");
        scanf("%d", &option);

        if (option == 0) {
            sendCommand(sd, "QUIT");
            break;
        } else if (option == 1) {
            createRoom(sd);
        } else if (option == 2) {
            listParticipants(sd);
        } else if (option == 3) {
            joinRoom(sd, &cliente);

            if (cliente.roomId != -1 && cliente.participantId != -1) {
                chat(sd, cliente);
                printf("%d %d", cliente.roomId, cliente.participantId);
            } else {
                printf("Erro ao entrar na sala. Verifique se o servidor atribuiu os IDs corretos.\n");
                printf("%d %d", cliente.roomId, cliente.participantId);
            }
        } else {
            printf("Opção inválida.\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Digite IP e Porta para este cliente\n");
        exit(1);
    }

    struct sockaddr_in endServ;
    int sd;

    if (inet_pton(AF_INET, argv[1], &endServ.sin_addr) <= 0) {
        perror("Conversão de endereço falhou");
        exit(1);
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        fprintf(stderr, "Falha ao criar socket!\n");
        exit(1);
    }

    endServ.sin_family = AF_INET;
    endServ.sin_port = htons(atoi(argv[2]));

    if (connect(sd, (struct sockaddr *)&endServ, sizeof(endServ)) < 0) {
        fprintf(stderr, "Conexão falhou\n");
        exit(1);
    }

    printf("Conectado ao servidor.\n");

    sendCommand(sd, "MENU");
    handleServerResponse(sd);
    showMenu(sd);

    close(sd);
    return EXIT_SUCCESS;
}

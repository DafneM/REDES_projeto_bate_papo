#ifndef CLIENTE_H_INCLUDED
#define CLIENTE_H_INCLUDED

#define MAX_SIZE 255
#define MAX_NAME_LENGTH 100
#define MAX_MESSAGE_LENGTH 200

typedef struct {
    char nome[MAX_NAME_LENGTH];
    int roomId;
    int participantId;
} clienteInfo;

void substitui_n(char *str);
void limpa_cmd();
void recebe_mensagem(int sd);
void handleChatMessage(const char *message);
void sendCommand(int sd, const char *command);
void handleServerResponse(int sd);
void createRoom(int sd);
void listParticipants(int sd);
void envia_mensagem(int sd, const char *message);
void joinRoom(int sd, clienteInfo *cliente);
void showMenu(int sd);
void chat(int sd, clienteInfo cliente);

#endif /* CLIENTE_H_INCLUDED */

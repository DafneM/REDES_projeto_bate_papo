#ifndef CLIENTE_H_INCLUDED
#define CLIENTE_H_INCLUDED

#define MAX_SIZE 255

typedef struct {
    char nome[50];
    int roomIndex;
} clienteInfo;

void substitui_n(char *str);
void limpa_cmd();
void envia_mensagem(int sd, clienteInfo cliente);
void recebe_mensagem(int sd);

#endif /* CLIENTE_H_INCLUDED */
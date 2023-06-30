#ifndef LIST
#define LIST

typedef struct clienteInfo
{
    int sd;
    char ip[16];
    char nome[50];
} clienteInfo;

typedef enum {
    PADRAO = 0,
    CRIAR_SALA = 1,
    ENTRAR_SALA = 2,
    SAIR_SALA = 3,
    LISTAR_SALAS = 4,
    ENVIAR_MENSAGEM = 5,
    SAIR = 6,
    CADASTRAR_USUARIO = 7,
} tipo_mensagem;

typedef struct Mensagem
{
    int tipo;
    char mensagem[256];
    char nome[50];
} Mensagem;

#endif // LIST
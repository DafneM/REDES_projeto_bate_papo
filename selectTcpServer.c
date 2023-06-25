#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>

#define QLEN            5               /* tamanho da fila de clientes  */
#define MAX_SIZE	80		/* tamanho do buffer */

void menu(){
    int opcao, limit_users, id_sala;
    char nome[1000];

    printf("Olá, digite o que você deseja fazer\n");
    printf("1 - Criar sala virtual\n");
    printf("2 - Listar participantes de uma sala\n");
    printf("3 - Entrar em uma sala\n");
    printf("4 - Criar sala\n");

    scanf("%d", opcao);
    if(opcao == 1){
        printf("Digite o nome que deseja para a sala\n");
        scanf("%s", nome);
        printf("Agora digite o limite de participantes para cada sala\n");
        scanf("%d", &limit_users);
    }
    else if(opcao == 2){
        printf("Digite o identificador sala que deseja que sejam listados os participantes\n");
        scanf("%d", &id_sala);
    }
    else if(opcao == 3){
        printf("Digite o identificador da sala que deseja entrar\n");
        scanf("%d", &id_sala);
    }
}

int main(int argc, char *argv[]) {
   struct sockaddr_in endServ;  /* endereco do servidor   */
   struct sockaddr_in endCli;   /* endereco do cliente    */
   int    sd, novo_sd;          /* socket descriptors */
   int    pid, alen,n; 

   fd_set sockets_atuais, aux_sockets_atuais; //select é destrutivo e vai mudar os sockets, precisamos de uma cópia
   int max_sd = sd;

   //inicializar o conjunto de sockets
   FD_ZERO(&sockets_atuais); // zerar conjunto de sockets atuais
   FD_SET(sd, &sockets_atuais);

   if (argc<3) {
	  printf("Digite IP e Porta para este servidor\n");
	  exit(1); }

   memset((char *)&endServ,0,sizeof(endServ)); 
   endServ.sin_family 		= AF_INET;           	/* familia TCP/IP   */
   endServ.sin_addr.s_addr 	= inet_addr(argv[1]); 	/* endereco IP      */
   endServ.sin_port 		= htons(atoi(argv[2])); /* PORTA	    */

   /* Cria socket */
   sd = socket(AF_INET, SOCK_STREAM, 0);
   if (sd < 0) {
     fprintf(stderr, "Falha ao criar socket!\n");
     exit(1); }

   /* liga socket a porta e ip */
   if (bind(sd, (struct sockaddr *)&endServ, sizeof(endServ)) < 0) {
     fprintf(stderr,"Ligacao Falhou!\n");
     exit(1); }

   /* Ouve porta */
   if (listen(sd, QLEN) < 0) {
     fprintf(stderr,"Falhou ouvindo porta!\n");
     exit(1); }

   printf("Servidor ouvindo no IP %s, na porta %s ...\n\n", argv[1], argv[2]);
   /* Aceita conexoes */
   alen = sizeof(endCli);
   for ( ; ; ) {
    aux_sockets_atuais = sockets_atuais;

    if(select(max_sd + 1, &aux_sockets_atuais, NULL, NULL, NULL) < 0){
        perror("Erro no select");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i<max_sd;i++){
        if(FD_ISSET(i, &aux_sockets_atuais)){
            if(i == sd){
                novo_sd=accept(sd, (struct sockaddr *)&endCli, &alen);
                FD_SET(novo_sd, &sockets_atuais);
                if(novo_sd > max_sd){
                    max_sd = novo_sd;
                }
            }
            else{
	            atende_cliente(novo_sd, endCli);
                FD_CLR(i, &sockets_atuais);
            }
        }
    }
   } 
   return EXIT_SUCCESS;
}

int atende_cliente(int descritor, struct sockaddr_in endCli)  {
   char bufin[MAX_SIZE];
   int  n;
   while (1) {
 	memset(&bufin, 0x0, sizeof(bufin));
	n = recv(descritor, &bufin, sizeof(bufin),0);
	if (strncmp(bufin, "FIM", 3) == 0)
            break;
	fprintf(stdout, "[%s:%u] => %s\n", inet_ntoa(endCli.sin_addr), ntohs(endCli.sin_port), bufin);
   } /* fim while */
   fprintf(stdout, "Encerrando conexao com %s:%u ...\n\n", inet_ntoa(endCli.sin_addr), ntohs(endCli.sin_port));
   close (descritor);
 } /* fim atende_cliente */

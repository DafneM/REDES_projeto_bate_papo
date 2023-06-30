# REDES_projeto_bate_papo

Para rodar:

```bash
gcc -o chatroom chatroom.c && ./chatroom 127.0.0.1 8080
```

Abrir outras abas no terminal e rodar:

```bash
gcc -o cliente cliente.c && ./cliente 127.0.0.1 8080
```

e começar a digitar e enviar as mensagens teclando o ENTER.

Ou

```bash
chmod +x app.sh
./app.sh
```
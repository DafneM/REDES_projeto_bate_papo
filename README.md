# REDES_projeto_bate_papo

Para rodar:


Executar com MakeFile

```bash
make clean
make
```

Abri os terminais e executa cliente e servidor

```bash
./cliente 127.0.0.1 8080
```
```bash
./chatroom 127.0.0.1 8080
```

Ou

```bash
gcc -o chatroom chatroom.c && ./chatroom 127.0.0.1 8080
```

Abrir outras abas no terminal e rodar:

```bash
gcc -o cliente cliente.c && ./cliente 127.0.0.1 8080
```

e começar a digitar e enviar as mensagens teclando o ENTER.


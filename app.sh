#!/bin/bash

# Comando para abrir uma nova aba de terminal no GNOME Terminal
gnome-terminal --tab --title="Chatroom" -- bash -c "make clean && make all && ./chatroom 127.0.0.1 8080"

# Aguarde alguns segundos para garantir que o chatroom esteja em execução
sleep 2

# Abra as outras duas abas de terminal
gnome-terminal --tab --title="Cliente 1" -- bash -c "./cliente 127.0.0.1 8080"
gnome-terminal --tab --title="Cliente 2" -- bash -c "./cliente 127.0.0.1 8080"

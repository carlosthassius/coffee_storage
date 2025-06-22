#!/bin/bash

# Verifica se o parâmetro foi fornecido
if [ -z "$1" ]; then
  echo "Uso: $0 <porta_serial>"
  echo "Exemplo: $0 /dev/ttyUSB0"
  exit 1
fi

# Caminho para o tunslip6 e porta serial passada como argumento
TUNSLIP="$HOME/contiki/tools/tunslip6"
SERIAL_PORT="$1"
IP_PREFIX="fd00::1/64"

# Loop infinito até o usuário parar com Ctrl+C
while true; do
    echo "Iniciando tunslip6 na porta $SERIAL_PORT..."
    sudo "$TUNSLIP" -v2 -s "$SERIAL_PORT" "$IP_PREFIX"

    echo "tunslip6 terminou. Reiniciando em 3 segundos..."
    sleep 3
done

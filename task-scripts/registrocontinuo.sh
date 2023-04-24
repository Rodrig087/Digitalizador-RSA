#!/bin/bash

# Dependiendo de los parámetros que se le pasen al programa se usa una opción u otra
case "$1" in
  start)
    echo "Arrancando sistema de registro continuo..."
    sudo killall -q digitalizador
    sudo /home/rsa/ejecutables/digitalizador &
    sleep 10
    python3 /home/rsa/ejecutables/SubirRegistroDrive.py &
    ;;
  stop)
    echo "Deteniendo sistema de registro continuo..."
    sudo killall -q digitalizador
    ;;
  *)
    echo "Modo de uso: registrocontinuo start|stop"
    exit 1
    ;;
esac
 
exit 0 

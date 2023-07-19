#!/bin/bash

# Compila todos los programas de la carpeta Programas 
g++ /home/rsa/Digitalizador-RSA/programas/Digitalizador_*.cpp -o /home/rsa/ejecutables/digitalizador -lbcm2835 -lwiringPi
gcc /home/rsa/Digitalizador-RSA/programas/ComprobarRegistro_*.c -o /home/rsa/ejecutables/comprobarregistro
gcc /home/rsa/Digitalizador-RSA/programas/ExtraerEventoBin_*.c -o /home/rsa/ejecutables/extraerevento
gcc /home/rsa/Digitalizador-RSA/programas/ResetMaster.c -o /home/rsa/ejecutables/resetmaster -lbcm2835 -lwiringPi 
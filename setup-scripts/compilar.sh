#!/bin/bash

# Compila todos los programas de la carpeta Programas 
g++ /home/rsa/Acelerografo-RSA/programas/Digitalizador_2022-05-12_VALE.cpp -o /home/rsa/ejecutables/digitalizador -lbcm2835 -lwiringPi
gcc programas/ComprobarRegistro_V3.c -o /home/rsa/ejecutables/comprobarregistro
gcc programas/ExtraerEventoBin_V2.c -o /home/rsa/ejecutables/extraerevento
gcc /home/rsa/Acelerografo-RSA/programas/ResetMaster.c -o /home/rsa/ejecutables/resetmaster -lbcm2835 -lwiringPi 
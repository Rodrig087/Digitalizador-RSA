#!/bin/bash

# Copiar el directorio "Configuracion" a la ruta /home/rsa/
cp -r configuracion /home/rsa/

# Crea los directorios necesarios
mkdir -p /home/rsa/ejecutables
mkdir -p /home/rsa/log-files
mkdir -p /home/rsa/tmp
mkdir -p /home/rsa/resultados/eventos-detectados
mkdir -p /home/rsa/resultados/eventos-extraidos
mkdir -p /home/rsa/resultados/registro-continuo
mkdir -p /home/rsa/resultados/consumo

# Crea los archivos necesarios
echo $(date) > /home/rsa/resultados/registro-continuo/nueva-estacion.txt
echo 'nueva-estacion.txt' > /home/rsa/tmp/NombreArchivoRegistroContinuo.tmp

# Compila todos los programas escritos en C 
g++ /home/rsa/Acelerografo-RSA/programas/Digitalizador_2022-05-12_VALE.cpp -o /home/rsa/ejecutables/digitalizador -lbcm2835 -lwiringPi
gcc /home/rsa/Acelerografo-RSA/programas/ComprobarRegistro_V3.c -o /home/rsa/ejecutables/comprobarregistro
gcc /home/rsa/Acelerografo-RSA/programas/ExtraerEventoBin_V2.c -o /home/rsa/ejecutables/extraerevento
gcc /home/rsa/Acelerografo-RSA/programas/ResetMaster.c -o /home/rsa/ejecutables/resetmaster -lbcm2835 -lwiringPi 

# Copia todos los programas escritos en Python a la carpeta /home/rsa/ejecutables
cp /home/rsa/Acelerografo-RSA/programas/*.py /home/rsa/ejecutables/

# Copia los task-scripts al directorio /usr/local/bin
sudo cp task-scripts/comprobar.sh /usr/local/bin/comprobar
sudo cp task-scripts/informacion.sh /usr/local/bin/informacion 
sudo cp task-scripts/registrocontinuo.sh /usr/local/bin/registrocontinuo

# Conceder permisos de ejecucion a los task-scripts
sudo chmod +x /usr/local/bin/comprobar
sudo chmod +x /usr/local/bin/informacion
sudo chmod +x /usr/local/bin/registrocontinuo
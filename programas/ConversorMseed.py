# -*- coding: utf-8 -*-
"""
Created on Jul 12 2023

@author: Milton Munoz
"""

# Programa que convierte los archivos binarios a mseed

# import tkinter
# from tkinter import filedialog
import os
import struct
import math, time
import numpy as np
import csv
from obspy import UTCDateTime, read, Trace, Stream
import datetime
import subprocess

#------------------------------------------------------------------------------
# Datos de configuracion
#------------------------------------------------------------------------------
fSample = 100
# Factor de conversion, si se coloca en 1 significa que no se aplica ningun factor
# y guarda un archivo de texto con 4 columnas de datos (ganancia, CH1, CH2, CH3)
# y todos como enteros
# Pero si se coloca el factor de conversion, se aplica a los datos y solo guarda 
# los tres canales en tres columnas como punto flotante. Siempre se guarda una
# primera columna con el tiempo
factor_conversion = 52428.8
factor_conversion = 1
#------------------------------------------------------------------------------

# ///////////////////////////////// Archivos //////////////////////////////////

archivoConfiguracionMseed = '/home/rsa/configuracion/configuracion_mseed.csv'
archivoNombresArchivosRC = '/home/rsa/tmp/NombreArchivoRegistroContinuo.tmp'
archivoDatosConfiguracion = '/home/rsa/configuracion/DatosConfiguracion.txt'

ficheroConfiguracion = open(archivoDatosConfiguracion)
ficheroNombresArchivos = open(archivoNombresArchivosRC)
    
lineasFicheroConfiguracion = ficheroConfiguracion.readlines()
lineasFicheroNombresArchivos = ficheroNombresArchivos.readlines()
    
nombreArchvioRegistroContinuo = lineasFicheroNombresArchivos[1].rstrip('\n')
print(nombreArchvioRegistroContinuo)
archivo_binario = lineasFicheroConfiguracion[2].rstrip('\n') + nombreArchvioRegistroContinuo
print(archivo_binario)

# /////////////////////////////////////////////////////////////////////////////


#------------------------------------------------------------------------------
# Seleccion del archivo binario a convertir:
#------------------------------------------------------------------------------
#abre una ventana para seleccionar el archivo
# root = tkinter.Tk()
# root.withdraw() # use to hide tkinter window
# # Definir los tipos de archivo y sus extensiones correspondientes
# tipos_archivo = (
    # ("Archivos binarios", "*.dat"),
    # ("Todos los archivos", "*.*")
# )
# # establecer atributos de la ventana de selección de archivos
# root.attributes('-topmost', True)
# root.attributes('-topmost', False)
# currdir = os.getcwd()
# archivo_binario = filedialog.askopenfilename(parent = root, initialdir = currdir, title = 'please select a file',filetypes=tipos_archivo)
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Funciones
#------------------------------------------------------------------------------

def parametros_():
#################################################################
#Método de Extracción de datos de conficuración de la estacion
#Desde el archivo configuracion.csv que debe estar presente en el mismo directorio del ejecutable.
        
        estaciones_=[]
        with open(archivoConfiguracionMseed,newline='') as f:
            datos=csv.reader(f,delimiter=';',quotechar=';')
            for r in datos:
                estaciones_.append(r)
        nombre_canal_total_=estaciones_[1][1]
        nombre_canal=estaciones_[1][2]
        tipo_canal_=estaciones_[1][3]
        n_canales_=estaciones_[1][4]
        hab_canal=estaciones_[1][5]
        componente_canal=estaciones_[1][6]
        grafico_=estaciones_[1][7]
        hab_plt=estaciones_[1][8]
        gan_plt=estaciones_[1][9]
        ganancia=estaciones_[1][10]
        diez_plt=estaciones_[1][11]
        bits_=estaciones_[1][12]
        calidad_=estaciones_[1][13]
        ubicacion_=estaciones_[1][14]
        tipo_canal=estaciones_[1][15]
        red_=estaciones_[1][16]
        muestreo_=estaciones_[1][17]
        longitud_=estaciones_[1][18]
        latitud_=estaciones_[1][19]
        altura_=estaciones_[1][20]
        ruido_=int(estaciones_[1][21])
        numero_est=int(estaciones_[1][0])
        filtro_=estaciones_[1][22]
        reserva_1=estaciones_[1][23]
        reserva_2=estaciones_[1][24]            
                
                
        return(nombre_canal_total_, #Canal 0  'NOMBRE'
               nombre_canal,        #Canal 1  'CODIGO'
               tipo_canal_,         #Canal 2  'SENSOR'
               n_canales_,          #Canal 3  'CANALES'
               hab_canal,           #Canal 4  'HAB_CANAL'
               componente_canal,    #Canal 5  'COMPONENTE'
               grafico_,            #Canal 6  'HAB_GRAFICO'
               hab_plt,             #Canal 7  'HAB_PLT'
               gan_plt,             #Canal 8  'GAN_PLT'
               ganancia,            #Canal 9  'GANANCIA'
               diez_plt,            #Canal 10 'DIEZ_PLT'
               bits_,               #Canal 11 'FACTOR_MUL'
               longitud_,           #Canal 12 'LONGITUD'
               latitud_,            #Canal 13 'LATITUD'
               altura_,             #Canal 14 'ALTITUD'
               ruido_,              #Canal 15 'RUIDO'
               calidad_,            #Canal 16 'CALIDAD'
               ubicacion_,          #Canal 17 'UBICACIÓN'
               tipo_canal,          #Canal 18 'CANAL'
               red_,                #Canal 19 'RED'
               muestreo_,           #Canal 20 'MUESTREO'
               numero_est,          #Canal 21 'N° ESTACION'
               filtro_,             #Canal 22 'FILTRO' 
               reserva_1,           #Canal 22 'Reserva 1'            
               reserva_2)           #Canal 22 'Reserva 2'


def obtenerTraza(nombreCanal,num_canal, data, anio, mes, dia, horas, minutos, segundos, microsegundos):#Depurado
    # Define todas las caracteristicas de la traza
    parametros=parametros_()
    nombreRed=parametros[19]
    nombreEstacion=parametros[1]
    tipoEstacion=parametros[2]
    localizacion=parametros[17]
    nCanal=parametros[18]
    fsample=int(parametros[20])
    calidad=parametros[16]
    if fsample>80:
        nombreCanal='E'
    else:
        nombreCanal='S'
    if tipoEstacion=='SISMICO':
        nombreCanal=nombreCanal+'L'
    else:
        nombreCanal=nombreCanal+'N'
    num_canal=num_canal-3*(int((num_canal-1)/3))
    nombreCanal=nombreCanal+nCanal[num_canal-1:num_canal]
    stats = {'network': nombreRed, 'station': nombreEstacion, 'location': localizacion,
             'channel': nombreCanal, 'npts': len(data), 'sampling_rate': fsample,
             'mseed': {'dataquality': calidad}}
    # Establece el tiempo
    stats['starttime'] = UTCDateTime(anio, mes, dia, horas, minutos, segundos, microsegundos)    
    # Crea la traza con los datos las caracteristicas
    traza = Trace(data = data, header = stats)
    return traza

def lectura_archivo(filepath): #archivo 
    print ("Archivo ", filepath)
    # Obtiene el nombre del archivo
    vectorFilepath = filepath.split('/')
    # El nombre esta en la ultima posicion
    fileNameExt = vectorFilepath[len(vectorFilepath) - 1]
    # Quita la extension porque no interesa
    fileName = fileNameExt[0 : (len(fileNameExt) - 4)]
    print(fileName)
    # Este es el tiempo de inicio
    anio = int(fileName[0:2])
    mes = int(fileName[2:4])
    dia = int(fileName[4:6])
    horas = int(fileName[6:8])
    minutos = int(fileName[8:10])
    contadorMinutosOk = minutos
    segundos = int(fileName[10:12])
    segundosAnt = segundos
    contadorSegundosOk = segundos
    print ('Hora Inicio ', anio, ' ', mes, ' ', dia, ' ', horas, ' ', minutos, ' ', segundos)
    
    # Los valores de AAMMDD en una sola variable corresponden a la cabecera
    fechaLong = 10000*anio + 100*mes + dia
    print('Fecha long ', fechaLong)
    
    # Abre el archivo para lectura de binarios "rb"
    objFile = open(filepath, "rb")
    
    # Lee el total de bytes del archivo, se guarda en un array de bytes
    bytesLeidos = objFile.read()
    numTotalBytes = len(bytesLeidos)
    
    # Se convierte el array de bytes a numeros, cada byte a numero
    # Primero se establece el formato, se debe incluir el total de bytes y B
    # corresponde a unsigned char (entero de 8 bits)
    formatoUnpack = '<' + str(numTotalBytes) + 'B'
    print (formatoUnpack)
    # Realiza la conversión de array de bytes a lista de enteros
    vectorDatos = struct.unpack(formatoUnpack, bytesLeidos)
    numTotalDatos = len(vectorDatos)
    print ('Total datos ', numTotalDatos)

    
    #crea la lista con 3 sublistas para guardar los datos de los 3 canales
    datos=[[],[],[]]
     
    # Recorre todos los bytes, hasta -15 porque siempre se necesitan al menos 
    # 8 bytes para registrar el tiempo y 7 de datos
    indiceDatos = 0
    while indiceDatos < (numTotalDatos - 8):
        # Se analiza si los 4 numeros siguientes son la cabecera (debe ser la fecha)
        valCuatroDatos = vectorDatos[indiceDatos] << 24 | vectorDatos[indiceDatos + 1] << 16 | vectorDatos[indiceDatos + 2] << 8 | vectorDatos[indiceDatos + 3]
        # Analiza si es igual a la fecha (que es la cabecera)
        if valCuatroDatos == fechaLong:
#            print("Fecha Ok indice ", indiceDatos)
            # Los cuatro siguientes datos son de la hora en segundos
            horaSegundos = vectorDatos[indiceDatos + 4] << 24 | vectorDatos[indiceDatos + 5] << 16 | vectorDatos[indiceDatos + 6] << 8 | vectorDatos[indiceDatos + 7]
            # Actualiza el indiceDatos sumando los 8 datos
            indiceDatos += 8
            # Coloca una bandera para leer los siguientes datos que son las 
            # muestras, hasta que aparezca de nuevo una cabecera
            banderaDatos = True
            contadorMuestreos = 0
            while banderaDatos == True:
                # Si los 4 primeros valores coinciden con la fecha, significa
                # cambio de minuto
                valCuatroDatos = vectorDatos[indiceDatos] << 24 | vectorDatos[indiceDatos + 1] << 16 | vectorDatos[indiceDatos + 2] << 8 | vectorDatos[indiceDatos + 3]                    
                # Si es la fecha, sale del bucle 
                if valCuatroDatos == fechaLong:
                    banderaDatos = False
                # Caso contrario almacena los 7 datos en los vectores mas el 
                # tiempo y cada vez que se completa los datos igual a la fsample
                # aumenta en 1 la hora segundos
                else:                    
                    # La ganancia corresponde a los 4 bits MSB del primer valor, 
                    # entonces se elimina los 4 bits LSB
                    ganancia = vectorDatos[indiceDatos] >> 4;
                    # EL canal 1 esta dividido: los 4 bits MSB en el primer dato 
                    # (parte LSB) y los 8 bits LSB en el segundo dato
                    valCH1 = ((vectorDatos[indiceDatos] & 0X0F) << 8) | vectorDatos[indiceDatos + 1];
                    # EL canal 2 esta dividido: los 4 bits MSB en el tercer dato 
                    # (parte MSB) y los 8 bits LSB en el cuarto dato
                    valCH2 = ((vectorDatos[indiceDatos + 2] >> 4) << 8) | vectorDatos[indiceDatos + 3];
                    # EL canal 3 esta dividido: los 4 bits MSB en el tercer dato 
                    # (parte LSB) y los 8 bits LSB en el quinto dato
                    valCH3 = ((vectorDatos[indiceDatos + 2] & 0X0F) << 8) | vectorDatos[indiceDatos + 4];
                    # Guarda todos los datos
                    # Si el factor de conversion es 1, guarda una columna adicional 
                    # de la ganancia y todo como enteros
                    if factor_conversion != 1:
                        valCH1 = valCH1/factor_conversion
                        valCH2 = valCH2/factor_conversion
                        valCH3 = valCH3/factor_conversion
                                            
                    #Guarda los valores de valCH1, valCH2 y valCH1 en la lista datos:
                    datos[0].append(int(valCH1))
                    datos[1].append(int(valCH2))
                    datos[2].append(int(valCH3))
                    
                    # Aumenta el indice en 5, que es el num datos por muestra
                    indiceDatos += 5
                    # Si el numero de datos es mayor que el total - 5 significa 
                    # que se han terminado los datos
                    if indiceDatos > (numTotalDatos - 5):
                        banderaDatos = False
                    
                    # Aumenta el contador de muestreos y si es igual a la 
                    # fsample significa que hay que aumentar en 1 los segundos
                    contadorMuestreos += 1
                    if contadorMuestreos == fSample:
                        horaSegundos += 1
                        contadorMuestreos = 0
        else:
            # Cada 1000 datos muestra el mensaje. Hay que mencionar que 1 segundo
            # tiene 30000 datos
            if indiceDatos%1000 == 0:
                print ("Fecha error ", valCuatroDatos, " indice ", indiceDatos)
#            time.sleep(1)
            indiceDatos += 1
#        print (indiceDatos)
                    
    # Al final cierra el archivo de lectura
    objFile.close()
    
    #convierte la lista datos en un arreglo NumPy
    datos_np = np.asarray(datos) 
    return (datos_np) 
      
   
def verificacion_archivo(filepath):
    # Obtiene el nombre del archivo
    vectorFilepath = filepath.split('/')
    # El nombre esta en la ultima posicion
    fileNameExt = vectorFilepath[len(vectorFilepath) - 1]
    # Quita la extension porque no interesa
    fileName = fileNameExt[0 : (len(fileNameExt) - 4)]
    print(fileName)
    # Este es el tiempo de inicio
    anio = int(fileName[0:2])
    mes = int(fileName[2:4])
    dia = int(fileName[4:6])
    horas = int(fileName[6:8])
    minutos = int(fileName[8:10])
    segundos = int(fileName[10:12])
    n_segundo=horas*3600+minutos*60+segundos
    # Tiempo en formato string
    anio_s= str(anio)
    mes_s=str(mes)
    if(mes<10):
        mes_s='0'+mes_s
    dia_s=str(dia)
    if(dia<10):
        dia_s='0'+dia_s
    hora_s=str(horas)
    if(horas<10):
        hora_s='0'+hora_s
    minuto_s=str(minutos)
    if(minutos<10):
        minuto_s='0'+minuto_s
    segundo_s=str(segundos)
    if(segundos<10):
        segundo_s='0'+segundo_s
    
    #print ('Hora Inicio ', anio, ' ', mes, ' ', dia, ' ', horas, ' ', minutos, ' ', segundos)
    
    fecha_=(anio,mes,dia,horas,minutos,segundos,n_segundo),(anio_s,mes_s,dia_s,hora_s,minuto_s,segundo_s)
    return(fecha_)


def nombre_mseed(nombre_,fecha_):
    anio_s=fecha_[1][0]
    mes_s=fecha_[1][1]
    dia_s=fecha_[1][2]
    hora_s=fecha_[1][3]
    minuto_s=fecha_[1][4]
    segundo_s=fecha_[1][5]
    fecha_string=anio_s+mes_s+dia_s        
    hora_string=hora_s+minuto_s+segundo_s
    #fileName = nombre_+fecha_string+"_"+hora_string+".mseed" 
    fileName = '/home/rsa/resultados/mseed/'+nombre_+fecha_string+"_"+hora_string+".mseed" 
    return fileName
    
def conversion_mseed_digital(fileName,fecha_,data_np):
    # Nombre del archivo en funcion del tiempo de inicio
    anio=fecha_[0][0]
    mes=fecha_[0][1]
    dia=fecha_[0][2]
    horas=fecha_[0][3]
    minutos=fecha_[0][4]
    segundos=fecha_[0][5]
    parametros=parametros_()
    nombre_=parametros[2]

    #fileName=self.directorio_registros+fileName
    
    # Una vez que se tiene los datos, llama al metodo para obtener la traza
    # Todos los parametros que recibe se detallan en el metodo (mas abajo)
    trazaCH1 = obtenerTraza(nombre_,1, data_np[0],(2000 + anio), mes, dia, horas, minutos, segundos, 0)
    trazaCH2 = obtenerTraza(nombre_,2, data_np[1],(2000 + anio), mes, dia, horas, minutos, segundos, 0)        
    trazaCH3 = obtenerTraza(nombre_,3, data_np[2],(2000 + anio), mes, dia, horas, minutos, segundos, 0)
    # Crea un objeto Stream con la traza
    #stData = Stream(traces=[trazaCH1])
    # Si se desea varias trazas, esto seria para cuando se tiene 3 canales
    stData = Stream(traces=[trazaCH1, trazaCH2, trazaCH3])
    
    # Guarda todas las trazas en un archivo en formato miniseed con codificacion
    # STEIM1 para disminuir el tamaño del archivo
    #nombreMseed = fileName + ".mseed"

    stData.write(fileName, format = 'MSEED', encoding = 'STEIM1', reclen = 512)

       
parametros=parametros_()
nombre=parametros[1]
fecha_=verificacion_archivo(archivo_binario)
nombre_= parametros[1]+'_20'
n_mseed=nombre_mseed(nombre_,fecha_)
datos_np=lectura_archivo(archivo_binario)
conversion_mseed_digital(n_mseed,fecha_,datos_np)

print(n_mseed)

subprocess.run(["python3", "/home/rsa/ejecutables/SubirArchivoDrive.py", "3", n_mseed])



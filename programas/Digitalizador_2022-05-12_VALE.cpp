
// Primero se debe instalar las librerias wiringPi para los pines GPIO y
// BCM2835 para la comunicacion SPI

// Para compilar y  ejecutar el programa
// Si se utiliza el terminal:
// Para compilar:
// g++ -Wall -o ExampleBCM2835-spi ExampleBCM2835-spi.cpp -lbcm2835 -lwiringPi
// g++ /home/rsa/Programas/Digitalizador_2022-05-12_VALE.cpp -o /home/rsa/Ejecutables/digitalizador -lbcm2835 -lwiringPi
// Para ejecutar:
// sudo ./ExampleBCM2835-spi

// Si se utiliza el editor CodeBlocks
// Primero se debe abrir como administrador, desde el terminal
// sudo codeblocks
// Luego hay que agregar las librerias al compilador
// En Settings-->Compiler buscar Linker settings y agregar las librerias
// /usr/lib/libwiringPi.so
// /usr/lib/libwiringPiDev.so
// /usr/local/lib/ libbcm2835.a
// Finalmente, solo con F9 se compila y ejecuta

// Si se compila con F9 desde el editor Geany
// Se debe ir a: Construir-->Establecer comandos de construcción
// Y en Build colocar: g++ -Wall -o "%e" "%f" -lwiringPi
// Además, se debe dar permisos de administrador, entonces en execute
// sudo "./%e"
// Luego se compila con F9 y se ejecuta con F5

//*************************************************************************************************
// Librerias
//*************************************************************************************************
#include <iostream>

#include <chrono>

// Libreria para manejar los pinesGPIO
#include <wiringPi.h>
// Libreria para la comunicacion SPI
#include <bcm2835.h>
// Libreria para gestionar el tiempo: time_t, struct tm, time, localtime
#include <time.h>
#include <string>
//*************************************************************************************************
// Fin Librerias
//*************************************************************************************************

using namespace std;

//*************************************************************************************************
// Declaracion de constantes
//*************************************************************************************************
// Pin GPIO 0 que corresponde al #11
#define PIN_INT_PIC 0
// Pin GPIO 26 que corresponde al #32
#define PIN_LED 26
// Pin GPIO 27 que corresponde al #36
#define PIN_LED_2 27
// PIN GPIO 28 que corresponde al #38, para el MCLR del dsPIC
#define PIN_MCLR 28
// Velocidad del SPI
#define FREQ_SPI 2000000
// Retardo despues de enviar un dato por el SPI, en micro segundos
#define TIEMPO_SPI 10
// Constante de time out del GPS en el caso de que no se conecte (en segundos)
// Si pasado este tiempo no se conecta, igual comienza el muestreo con el RTC
#define TIME_OUT_GPS 100

// Constantes para la comunicacion entre la RPi y el dsPIC por SPI
// Valor dummy que no interesa, pero sirve para leer los datos del SPI, porque
// hay que enviar uno dummy. Este valor lo envia la RPi para leer datos del dsPIC
#define DUMMY_BYTE 0X00
// Valores de inicio y fin para obtener la operacion que desea el dsPIC
#define INI_OBT_OPE 0XA0
#define FIN_OBT_OPE 0XF0
// Valores de inicio y fin para el envio del tiempo desde el RPi al micro
#define INI_TIME_FROM_RPI 0XA4
#define FIN_TIME_FROM_RPI 0XF4
// Valores de inicio y fin para el envio del tiempo desde el dsPIC a la RPi
#define INI_TIME_FROM_DSPIC 0xA5
#define FIN_TIME_FROM_DSPIC 0XF5
// Valores de inicio y fin para indicar al dsPIC que comience el muestreo
#define INI_INIT_MUES 0XA1
#define FIN_INIT_MUES 0XF1
// Valores de inicio y fin para recibir los bytes de un muestreo desde dsPIC
#define INI_REC_MUES 0XA3
#define FIN_REC_MUES 0XF3
// Indica que se desea enviar bytes de cierto numero de muestreos a la RPi
#define ENV_MUESTRAS 0XB1
// Indica que se desea enviar tanto los bytes de cierto numero de muestreos, junto
// con el tiempo, esto es cada cambio de segundo
#define ENV_MUESTRAS_TIME 0XB2
// Indica que se desea enviar el tiempo del sistema a la RPi
#define ENV_TIME_SIS 0XB3
// Indica que el dsPIC se ha conectado y configurado
#define DSPIC_CONEC 0XB4
// Indica que el GPS esta conectado y se ha recibido los datos
#define GPS_OK 0XB5
// Define las dos fuentes de tiempo del sistema: 1 para GPS y 2 para RTC
#define FUENTE_TIME_GPS 0X01
#define FUENTE_TIME_RTC 0X02

// Define la frecuencia de muestreo
#define fsample 100
// Numero de bytes por muestra, 7 en total, 1 de ganancia y 6 de CH1, CH2 y CH3
//#define numBytesPorMuestra 7
// Numero de bytes a recibir por SPI, si se considera 5 muestreos: 5x7 = 35
//#define numMuestrasEnvio 35
//#define numMuestrasEnvio 7
// Numero de bytes por muestra, 5 en total, se consideran los bits de cada variable
// Entonces un dato con los 4 bits de la ganancia y 4 bits MSB CH1, otro dato con
// los 8 bits LSB del CH1, otro dato con los 4 bits MSB de los CH2 y CH3, por ultimo
// dos datos con los 8 bits LSB de los CH2 y CH3
#define numBytesPorMuestra 5
// Numero de bytes a enviar por SPI, si se considera 10 muestreos: 10x5 = 50
//#define numMuestrasEnvio 50
#define numMuestrasEnvio 250
// Variable que indica el indice de inicio para almacenar los datos recibidos del dsPIC en el vector
// Se comienza en 5, porque los primeros 5 valores corresponden a 4 bytes de la fecha tipo long
// Y el 5to al valor MSB de la hora en long que siempre es 0, por eso no se envia desde el dsPIC
//#define numBytesFechaMasHoraMSB 5
#define numBytesFechaMasHoraMSB 2
// Bytes de tiempo a recibir, unicamenente son 6. 3 que corresponden a los bytes menos significativo
// de la hora en formato long y 3 a la fecha en formato long
const unsigned char numBytesHoraToRec = 6;
//const unsigned char numBytesTiempo = 3;
// Contador del total de muestras, para saber cuantas hay que guardar en el archivo de texto
// El tiempo si bien se recibe al ultimo, se guarda en las primeras numBytesTiempo posiciones, por eso comienza el contador en numBytesTiempo
//unsigned int contadorDatosToSave = numBytesFechaMasHoraMSB + numBytesHoraToRec;
unsigned int contadorDatosToSave = 0;
//*************************************************************************************************
// Fin Declaracion de constantes
//*************************************************************************************************

//*************************************************************************************************
// Declaracion de variables
//*************************************************************************************************
// Variable que indica si esta o no conectado el dsPIC
bool is_dsPIC_Connected = false;
// Variable que indica si esta o no conectado el GPS en el micro
bool is_GPS_Connected = false;
// Variable que ajusta el tiempo de time out en el caso de que el GPS no se conecte
unsigned char contadorTimeOutGPS = 0;
// Vector para almacenar los datos recibidos por SPI y que se van a guardar en el archivo de texto
// Se guardan en el archivo cada segundo, entonces el total de datos es fsample*numBytesPorMuestra + numBytesTiempoToRec
//const unsigned int totalDatosPorMin = 60*(fsample*numBytesPorMuestra) + numBytesFechaMasHoraMSB + numBytesHoraToRec;
const unsigned int totalDatosPorMin = 60 * (fsample * numBytesPorMuestra); //60*(100*5)=30000

// Declara el vector para recibir los datos con memoria dinámica, en el caso de que no se conozca la dimension
// Asi ptrVectorDatos apunta a la direccion inicial del vector
// Para acceder a los elementos se puede utilizar: ptrVectorDatos[1] o *(ptrVectorDatos + 1)
//unsigned char ptrVectorDatosToRec [numMuestrasEnvio + numBytesHoraToRec];
//unsigned char ptrVectorDatosToSave [totalDatosPorMin];
unsigned char *ptrVectorDatosToRec;
unsigned char *ptrVectorDatosToSave;
// Vector para almacenar los bytes de las dos variables long de tiempo recibidas por el dsPIC
unsigned char vectorBytesTimeDSPIC[8];
// Vector para almacenar el tiempo recibido por el dsPIC
unsigned char vectorTimeDSPIC[6];

// Objeto tipo File para el archivo donde se guardan los datos
FILE *objFile;
// Objeto tipo File para el archivo donde se guardan las lecturas instantaneas del canal
FILE *fTmpCanal;

// String con el path donde se van a crear los archivos, tambien la extension y el formato del nombre de archivo
string path = "/home/rsa/Resultados/RegistroContinuo/", extFile = ".dat", nombreArchivo = "YYMMDDhhmmss";
// Declara el ptrArchivoCompleto con memoria dinamica, este contendra el path + nombreArchivo + extFile
string *ptrArchivoCompleto;
// Bandera que se activa al inicio del muestreo o cuando hay cambio de dia, para crear un nuevo archivo
bool isCrearNuevoArchivo = false;

// Variables tipo long para el tiempo
unsigned long fechaLongDSPIC, horaLongDSPIC, fechaAnteriorComp = 0;
//*************************************************************************************************
// Fin Declaracion de variables
//*************************************************************************************************

//*************************************************************************************************
// Declaracion de funciones
//*************************************************************************************************
int Setup();
void ResetearDSPIC();
void ObtenerOperacion();
void RecibirBytesMuestra(bool incluyeTiempo);
void GuardarDatosEnArchivo(unsigned char *vectorDatosGuardar, unsigned int numDatosGuardar);
void GuardarDatosCanal(unsigned int valCH1, unsigned int valCH2, unsigned int valCH3);
void IniciarMuestreo();
void EnviarTiempoLocal();
void PasarTiempoToVector(unsigned long longHora, unsigned long longFecha, unsigned char *tramaTiempoSistema);
string PasarHoraLongToString(unsigned long longHora);
bool ObtenerTiempoDSPIC();
//*************************************************************************************************
// Fin Declaracion de funciones
//*************************************************************************************************

//*************************************************************************************************
// Metodo principal
//*************************************************************************************************
int main(void)
{
    int resultConf;
    bool resultTimeDSPIC = false;
    unsigned short dummyRec;

    cout << "Inicio Programa" << endl;

    // Llama al metodo para configurar la libreria wiringPi, la BCM2835 y el SPI
    // Si devuelve 1 significa que existio algun error
    resultConf = Setup();
    if (resultConf == 1)
    {
        return 1;
    }
    // Retardo para establecer las configuraciones y esperar que el dsPIC tambien se configure
    delay(10000);

    // En el metodo Setup ya se resetea el dsPIC y debe responder
    // pero en el caso de que aun no este conectado, lo resetea de nuevo
    while (is_dsPIC_Connected == false)
    {
        cout << "El dsPIC esta desconectado, reiniciando..." << endl;
        ResetearDSPIC();
        delay(10000);
    }

    // Tiempo de inicio del envio
    //    auto start = chrono::high_resolution_clock::now();

    // Una vez que se ha conectado y configurado el dsPIC
    // Envia el tiempo actual de la RPi. Esto es util en el caso de que no se conecte el GPS
    if (is_GPS_Connected == false)
    {
        EnviarTiempoLocal();
        delay(1000);
    }

    // Obtiene el tiempo que ha pasado desde el inicio del metodo
    //    auto elapsed = chrono::high_resolution_clock::now() - start;
    //    long long microseconds = chrono::duration_cast < chrono::microseconds > (elapsed).count();
    //    cout << "Tiempo en us: " << to_string(microseconds) << endl;

    // Espera un tiempo para que se conecte el GPS (30 segundos aproximadamente), si en este
    // tiempo no se conecta el GPS, igual comienza la adquisicion aunque sera con el RTC
    if (is_GPS_Connected == false)
    {
        cout << "Esperando conexion de GPS..." << endl;
    }
    while (is_GPS_Connected == false)
    {
        delay(1000);
        contadorTimeOutGPS++;
        // Si el contador supera el time out, termina el break para enviar los parametros de comienzo de muestreo
        if (contadorTimeOutGPS >= TIME_OUT_GPS)
        {
            contadorTimeOutGPS = 0;
            cout << "El GPS no se ha conectado" << endl;
            break;
        }
    }

    while (resultTimeDSPIC == false)
    {
        delay(1000);
        // Solicita el tiempo al dsPIC, en caso de que el GPS no se ha conectado seria el enviado por la RPi
        // y controlado por el RTC. Caso contrario el del GPS
        cout << "Solicitud de tiempo al dsPIC" << endl;
        // Llama al metodo para obtener el tiempo del dsPIC
        resultTimeDSPIC = ObtenerTiempoDSPIC();
    }

    //Prueba
    //GuardarDatosCanal(333, 666, 999);

    // Una vez recibido el tiempo, llama al metodo para enviar los comandos de inicio de muestreo
    IniciarMuestreo();

    // Bucle
    while (1)
    {
        delay(1000);
    }

    // Finaliza el SPI y la libreria BCM2835
    bcm2835_spi_end();
    bcm2835_close();

    return 0;
}
//*************************************************************************************************
// Fin Metodo principal
//*************************************************************************************************

//*************************************************************************************************
// Metodo para realizar las configuracion de variables, pines de entrada y salida, interrupciones,
// y comunicacion SPI
//*************************************************************************************************
int Setup()
{
    unsigned short dummyVal;

    // Inicio de la libreria wiringPi
    if (wiringPiSetup() < 0)
    {
        cout << "Error en la libreria WiringPi" << endl;
        return 1;
    }
    cout << "Libreria wiringPi iniciada correctamente" << endl;

    // Configuracion de los pines de entrada y salida
    // Pin del LED de prueba como salida
    pinMode(PIN_LED, OUTPUT);
    // Enciende el LED
    digitalWrite(PIN_LED, HIGH);
    // Pin del LED de prueba como salida
    pinMode(PIN_LED_2, OUTPUT);
    // Enciende el LED
    digitalWrite(PIN_LED_2, HIGH);

    // Configura el pin del MCLR del dsPIC como salida, para resetear el micro
    pinMode(PIN_MCLR, OUTPUT);
    // Llama al metodo para resetear el dsPIC
    ResetearDSPIC();

    // Pin de interrupcion del PIC como entrada
    pinMode(PIN_INT_PIC, INPUT);
    // Configura la interrupcion en el flanco ascendente y llama a la funcion ObtenerOperacion
    wiringPiISR(PIN_INT_PIC, INT_EDGE_RISING, ObtenerOperacion);

    // Reinicia el modulo SPI
    system("sudo rmmod  spi_bcm2835");
    bcm2835_delayMicroseconds(500);
    system("sudo modprobe spi_bcm2835");

    // Inicia la libreria bcm2835 y el SPI
    if (!bcm2835_init())
    {
        cout << ("bcm2835_init fallo. Ejecuto el programa como root?\n") << endl;
        return 1;
    }
    if (!bcm2835_spi_begin())
    {
        cout << ("bcm2835_spi_begin fallo. Ejecuto el programa como root?\n") << endl;
        return 1;
    }
    cout << "Libreria BCM2835 y SPI iniciados correctamente" << endl;

    // Configuracion de parametros del SPI
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);
    // Clock divider RPi 2
    //bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32);
    // Clock divider RPi 3
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);
    bcm2835_spi_set_speed_hz(FREQ_SPI);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    // Asigna un bloque de memoria a los vectores
    // Para el vector que recibe los datos por el SPI
    // Dimension en funcion del numero de bytes que se pueden recibir en un muestreo + los 3 bytes de la hora
    //    ptrVectorDatosToRec = new (nothrow) unsigned char [numMuestrasEnvio + numBytesHoraToRec];
    ptrVectorDatosToRec = new (nothrow) unsigned char[numMuestrasEnvio];
    // Nothrow permite determinar si se ha asignado correctamente la memoria o no
    if (ptrVectorDatosToRec == nullptr)
    {
        cout << "Error en asignacion de memoria ptrVectorDatosToRec " << endl;
    }

    // Para el vector que almacena los datos recibidos y guarda en el archivo de texto cada minuto
    ptrVectorDatosToSave = new (nothrow) unsigned char[totalDatosPorMin];
    // Nothrow permite determinar si se ha asignado correctamente la memoria o no
    if (ptrVectorDatosToSave == nullptr)
    {
        cout << "Error en asignacion de memoria ptrVectorDatosToSave " << endl;
    }

    // Mensajes de configuracion completa
    cout << "Configuracion Completa" << endl;

    // Devuelve 0, que significa todo Ok
    return 0;
}
//*************************************************************************************************
// Fin Metodo de COnfiguraciones
//*************************************************************************************************

void ResetearDSPIC()
{
    // Genera un pulso para resetear el dsPIC
    digitalWrite(PIN_MCLR, HIGH);
    delay(100);
    digitalWrite(PIN_MCLR, LOW);
    delay(100);
    digitalWrite(PIN_MCLR, HIGH);
}

//*************************************************************************************************
// Metodo para obtener la operacion que envia el dsPIC, este metodo se llama con la interrupcion del
// PIN_INT_PIC
//*************************************************************************************************
void ObtenerOperacion()
{
    unsigned short byteRecSPI;

    // Tiempo de inicio del envio
    //    auto start = chrono::high_resolution_clock::now();

    //    cout << "Solicitud dsPIC " << endl;

    // Debido a que en el dsPIC se espera 20us con el pin en alto, aqui tambien se hace un delay
    //    delayMicroseconds(15);
    delayMicroseconds(30);

    // Cambia el estado del LED
    digitalWrite(PIN_LED_2, !digitalRead(PIN_LED_2));

    // Envia el byte de inicio de obtener operacion
    bcm2835_spi_transfer(INI_OBT_OPE);
    bcm2835_delayMicroseconds(TIEMPO_SPI);
    // Recibe la operacion que desea realizar el micro
    byteRecSPI = bcm2835_spi_transfer(DUMMY_BYTE);
    bcm2835_delayMicroseconds(TIEMPO_SPI);
    // Envia el byte de fin de operacion
    bcm2835_spi_transfer(FIN_OBT_OPE);
    bcm2835_delayMicroseconds(TIEMPO_SPI);

    // Aqui se selecciona el tipo de operacion que se va a ejecutar
    // Si se ha recibido que el dsPIC se ha conectado y configurado
    if (byteRecSPI == DSPIC_CONEC)
    {
        cout << "Recibido dsPIC_CONEC " << DSPIC_CONEC << endl;
        // Actualiza el estado de la bandera dsPIC
        is_dsPIC_Connected = true;
        // Si se ha recibido GPS ok
    }
    else if (byteRecSPI == GPS_OK)
    {
        cout << "Recibido GPS_OK " << GPS_OK << endl;
        // Actualiza el estado de la bandera del GPS
        is_GPS_Connected = true;
        // Si se ha recibido enviar tiempo del sistema desde el dsPIC
    }
    else if (byteRecSPI == ENV_TIME_SIS)
    {
        cout << "Recibido ENV_TIME_SIS " << ENV_TIME_SIS << endl;
        // Llama al metodo para obtener el tiempo del dsPIC
        ObtenerTiempoDSPIC();
        // Si se ha recibido el envio de los bytes de varios muestreos desde el dsPIC
    }
    else if (byteRecSPI == ENV_MUESTRAS)
    {
        //        cout << "Recibido ENV_MUESTRAS " << ENV_MUESTRAS << endl;
        // Llama al metodo para obtener los bytes
        // Se envia false que indica que no incluye tiempo
        RecibirBytesMuestra(false);
        // Si se ha recibido el envio de los bytes de varios muestreos junto con el tiempo desde el dsPIC
    }
    else if (byteRecSPI == ENV_MUESTRAS_TIME)
    {
        //        cout << "Recibido ENV_MUESTRAS_TIME " << ENV_MUESTRAS_TIME << endl;
        // Llama al metodo para obtener los bytes de los muestreos y del tiempo
        // Se envia true que significa que incluye el tiempo
        RecibirBytesMuestra(true);
    }
    else
    {
        cout << "Otra operacion recibida " << byteRecSPI << endl;
    }

    // Obtiene el tiempo que ha pasado desde el inicio del metodo
    //    auto elapsed = chrono::high_resolution_clock::now() - start;
    //    long long microseconds = chrono::duration_cast < chrono::microseconds > (elapsed).count();
    //    if (microseconds > 1000) {
    //        cout << "Tiempo de envio en us: " << to_string(microseconds) << endl;
    //    }
}
//*************************************************************************************************
// Fin Metodo Obtener Operacion
//*************************************************************************************************

//*************************************************************************************************
// Metodo para recibir los bytes de un muestreo
//*************************************************************************************************
void RecibirBytesMuestra(bool incluyeTiempo)
{

    // Indice for para este metodo
    unsigned int indiceForRBM;
    // Variable para almacenar el dato recibido por el SPI
    unsigned char dataRecSPI;
    // Variable que indica cuantos bytes se deben recibir
    unsigned int numDatosToRec;
    // Contador para recibir los datos de tiempo, se reciben solo una vez por segundo asi que siempre se resetea
    // Se coloca en 5 porque los 4 primeros datos de la fecha se guardan los de la RPi y el primer byte MSB de
    // la hora siempre es 0, entonces a la final solo se reciben los 3 bytes de la hora
    unsigned char indiceDatosTime = 0;
    unsigned char vectorDatosHoraToRec[6];
    // Contador para recibir los bytes, siempre se resetea al valor de numBytesHoraToRec porque en las
    // primeras posiciones se guarda el tiempo (siempre que exista tiempo)
    //    unsigned int contadorDatosToRec = numBytesHoraToRec;
    unsigned int contadorDatosToRec = 0;

    // Variables para obtener los valores del primer muestreo y comprobar si son logicos
    unsigned char ganancia;
    unsigned int valCH1, valCH2, valCH3;

    // Variables unicamente para comparacion
    //    static unsigned long horaAnteriorComp = 0,
    static unsigned long horaAnteriorComp = 0;

    // Cambia el estado del LED
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));

    // Dependiendo si se desea o no recibir el tiempo, se define el numero de bytes a recibir
    if (incluyeTiempo == true)
    {
        numDatosToRec = numMuestrasEnvio + numBytesHoraToRec;
    }
    else
    {
        numDatosToRec = numMuestrasEnvio;
    }

    // Envia el delimitador de inicio de trama para recibir los bytes de un muestreo del dsPIC
    bcm2835_spi_transfer(INI_REC_MUES);
    bcm2835_delayMicroseconds(TIEMPO_SPI);

    // Guarda los bytes de la hora y fecha tipo long devuelta por el dsPIC
    for (indiceForRBM = 0; indiceForRBM < numDatosToRec; indiceForRBM++)
    {
        dataRecSPI = bcm2835_spi_transfer(DUMMY_BYTE);
        bcm2835_delayMicroseconds(TIEMPO_SPI);

        // Si el indice es menor que el numMuestrasEnvio, significa que es dato
        if (indiceForRBM < numMuestrasEnvio)
        {
            ptrVectorDatosToRec[contadorDatosToRec] = dataRecSPI;
            contadorDatosToRec++;
            // Caso contrario significa dato de tiempo, esto implica que se termino de recibir los datos del segundo en cuestion
        }
        else
        {
            //            ptrVectorDatosToRec[indiceDatosTime] = dataRecSPI;
            vectorDatosHoraToRec[indiceDatosTime] = dataRecSPI;
            indiceDatosTime++;
        }
    }

    // Envia el delimitador de final de trama para recibir los bytes de un muestreo del dsPIC
    bcm2835_spi_transfer(FIN_REC_MUES);
    bcm2835_delayMicroseconds(TIEMPO_SPI);

    //    if (contadorDatos >= (totalDatosPorSec - numBytesTiempo)) {
    //        GuardarDatosEnArchivo(ptrVectorDatos, contadorDatos);
    //        contadorDatos = 0;
    //    }

    // Obtiene los valores de ganancia y de los 3 canales del primer muestreo. Comienza en
    // el indice indiceInicioVectorDatos + numBytesTiempoToRec porque los primeros valores son del tiempo
    //    unsigned char indiceValores = numBytesHoraToRec;
    unsigned char indiceValores = 0;
    //    ganancia = ptrVectorDatos[indiceValores];
    //    valCH1 = (ptrVectorDatos[indiceValores + 1] << 8) | ptrVectorDatos[indiceValores + 2];
    //    valCH2 = (ptrVectorDatos[indiceValores + 3] << 8) | ptrVectorDatos[indiceValores + 4];
    //    valCH3 = (ptrVectorDatos[indiceValores + 5] << 8) | ptrVectorDatos[indiceValores + 6];

    // La ganancia corresponde a los 4 bits MSB del primer valor, entonces se elimina los 4 bits LSB
    ganancia = ptrVectorDatosToRec[indiceValores] >> 4;
    // EL canal 1 esta dividido: los 4 bits MSB en el primer dato (parte LSB) y los 8 bits LSB en el segundo dato
    valCH1 = ((ptrVectorDatosToRec[indiceValores] & 0X0F) << 8) | ptrVectorDatosToRec[indiceValores + 1];
    // EL canal 2 esta dividido: los 4 bits MSB en el tercer dato (parte MSB) y los 8 bits LSB en el cuarto dato
    valCH2 = ((ptrVectorDatosToRec[indiceValores + 2] >> 4) << 8) | ptrVectorDatosToRec[indiceValores + 3];
    // EL canal 3 esta dividido: los 4 bits MSB en el tercer dato (parte LSB) y los 8 bits LSB en el quinto dato
    valCH3 = ((ptrVectorDatosToRec[indiceValores + 2] & 0X0F) << 8) | ptrVectorDatosToRec[indiceValores + 4];
    //    cout << "Gan " << to_string(ganancia) << " CH1 " << to_string(valCH1) << " CH2 " << to_string(valCH2) << " CH3 " << to_string(valCH3) << endl;

    // Comprueba que los valores sean logicos, la ganancia no puede ser mas de 15 y los datos no pueden ser mas de 4095
    // Si no es asi, significa falla en la comunicacion
    if (ganancia > 15 || valCH1 > 4095 || valCH2 > 4095 || valCH3 > 4095)
    {
        cout << "Error datos " << endl;
        cout << "Gan " << to_string(ganancia) << " CH1 " << to_string(valCH1) << " CH2 " << to_string(valCH2) << " CH3 " << to_string(valCH3) << endl;
    }
    else
   
    //Guarda los las lecturas de los canales en un arcivo de texto:
    GuardarDatosCanal(valCH1, valCH2, valCH3);
    

    // Si se ha recibido tambien el tiempo
    if (incluyeTiempo == true)
    {

        // Si el contador de datos del vector para guardar en el archivo de texto es
        // diferente al total de datos por minuto, significa que hubo algun error durante ese minut0
        if (contadorDatosToSave != totalDatosPorMin)
        {
            cout << "Numero de datos por minuto diferente " << to_string(contadorDatosToSave) << endl;
            unsigned long horaLongDSPIC_aux = (vectorDatosHoraToRec[0] << 16) | (vectorDatosHoraToRec[1] << 8) | vectorDatosHoraToRec[2];
            unsigned long fechaLongDSPIC_aux = (vectorDatosHoraToRec[3] << 16) | (vectorDatosHoraToRec[4] << 8) | vectorDatosHoraToRec[5];
            cout << "Fecha Long " << to_string(fechaLongDSPIC_aux) << " Hora Long " << to_string(horaLongDSPIC_aux) << endl;
        }

        // Primero se guardan los datos
        // Como se recibio el tiempo, primero guarda el tiempo, que corresponde al minuto anterior
        // Recordar que siempre se guarda los datos anteriores porque se van almacenando, estos son 8 bytes
        // de las dos variables tipo long
        GuardarDatosEnArchivo(vectorBytesTimeDSPIC, (numBytesFechaMasHoraMSB + numBytesHoraToRec));
        // Luego, se guardan los datos en el archivo
        GuardarDatosEnArchivo(ptrVectorDatosToSave, contadorDatosToSave);
        // Luego resetea el contador
        //        contadorDatosToSave = numBytesFechaMasHoraMSB + numBytesHoraToRec;
        contadorDatosToSave = 0;

        // LA HORA TIENE QUE ACTUALIZARSE AQUI Y NO ANTES POR LA CREACION DE LOS ARCHIVOS, de no hay desfase de un minuto
        // Pasa los 4 bytes de la hora a una sola variable tipo long, se desplazan los bytes a las posiciones
        // respectivas, primero va el byte MSB (desplazado 32 bits). Como el MSB siempre es 0 no se considera
        //        horaLongDSPIC = (ptrVectorDatosToRec[5] << 16) | (ptrVectorDatosToRec[6] << 8) | ptrVectorDatosToRec[7];
        //        horaLongDSPIC = (ptrVectorDatosToRec[0] << 16) | (ptrVectorDatosToRec[1] << 8) | ptrVectorDatosToRec[2];
        horaLongDSPIC = (vectorDatosHoraToRec[0] << 16) | (vectorDatosHoraToRec[1] << 8) | vectorDatosHoraToRec[2];
        fechaLongDSPIC = (vectorDatosHoraToRec[3] << 16) | (vectorDatosHoraToRec[4] << 8) | vectorDatosHoraToRec[5];
        cout << "Fecha Long " << to_string(fechaLongDSPIC) << " Hora Long " << to_string(horaLongDSPIC) << endl;

        // Analiza si hubo cambio de dia, con un margen de 4 minutos
        if (fechaAnteriorComp != fechaLongDSPIC)
        {
            // Actualiza la fecha anterior
            fechaAnteriorComp = fechaLongDSPIC;
            // En este caso significa que hay que crear un nuevo archivo de texto
            isCrearNuevoArchivo = true;
        }

        // Si la hora es diferente con la que se tiene anteriormente significa que hubo un error
        if ((horaAnteriorComp + 60) != horaLongDSPIC)
        {
            cout << "Fecha Long " << to_string(fechaLongDSPIC) << " Hora DIF Long " << to_string(horaLongDSPIC) << endl;
            // Iguala las horas para comparar en el proximo segundo
            horaAnteriorComp = horaLongDSPIC;
            // Caso contrario todo ok y aumenta la referencia en 1 minuto
        }
        else
        {
            horaAnteriorComp = horaAnteriorComp + 60;
        }

        // Analiza que la fecha y la hora recibidas si sean logicas, caso contrario
        // significa que hubo error en la recepcion de datos
        // La hora obviamente no puede ser mayor que 86400 y considera que la fecha no
        // puede ser mayor que el 31-dec-2099
        //        if (horaLongDSPIC <= 86400 && fechaLongDSPIC <= 991231) {
        if (horaLongDSPIC <= 86400)
        {
            // Aqui se agregan tanto la fecha como la hora al vector de datos que se guardaran en el archivo
            // El byte MSB tanto de la fecha como de la hora, es 0
            // Primero la fecha
            vectorBytesTimeDSPIC[0] = 0;
            // El resto de datos se recibe desde el dsPIC
            vectorBytesTimeDSPIC[1] = vectorDatosHoraToRec[3];
            vectorBytesTimeDSPIC[2] = vectorDatosHoraToRec[4];
            vectorBytesTimeDSPIC[3] = vectorDatosHoraToRec[5];
            // Luego la hora
            vectorBytesTimeDSPIC[4] = 0;
            // El resto de datos se recibe desde el dsPIC
            vectorBytesTimeDSPIC[5] = vectorDatosHoraToRec[0];
            vectorBytesTimeDSPIC[6] = vectorDatosHoraToRec[1];
            vectorBytesTimeDSPIC[7] = vectorDatosHoraToRec[2];
        }
        else
        {
            cout << "Error en Fecha " << fechaLongDSPIC << " y Hora " << horaLongDSPIC << endl;
        }
        // Analiza un posible error, si no se recibe el tiempo y ya se ha superado el numero de datos
    }
    else
    {
        //        cout << "NumDatos " << contadorDatos << endl;
        if (contadorDatosToSave > (totalDatosPorMin - numMuestrasEnvio))
        {
            cout << "Se ha superado el numero de datos sin tiempo " << to_string(contadorDatosToSave) << endl;
            cout << "Fecha Long " << to_string(fechaLongDSPIC) << " Hora Long " << to_string(horaLongDSPIC) << endl;

            // Si toca crear un archivo nuevo y no se recibió el tiempo
            // Envia con el anterior. En este caso debería ser 0 porque ya
            // estariamos terminando el primer minuto de datos
            if (isCrearNuevoArchivo == true)
            {
                // Se guarda el tiempo, que corresponde al minuto anterior. Recordar que siempre
                // se guarda los datos anteriores porque se van almacenando, estos son 8 bytes
                // de las dos variables tipo long
                cout << "Entro" << endl;
                GuardarDatosEnArchivo(vectorBytesTimeDSPIC, (numBytesFechaMasHoraMSB + numBytesHoraToRec));
            }

            GuardarDatosEnArchivo(ptrVectorDatosToSave, contadorDatosToSave);
            // Luego resetea el contador
            //            contadorDatosToSave = numBytesFechaMasHoraMSB + numBytesHoraToRec;
            contadorDatosToSave = 0;
        }
    }

    // Al final guarda los valores de las muestras recibidas en el vector para el archivo
    //    for (indiceForRBM = numBytesHoraToRec; indiceForRBM < (numMuestrasEnvio + numBytesHoraToRec); indiceForRBM++) {
    for (indiceForRBM = 0; indiceForRBM < numMuestrasEnvio; indiceForRBM++)
    {
        ptrVectorDatosToSave[contadorDatosToSave] = ptrVectorDatosToRec[indiceForRBM];
        // Incrementa el contador
        contadorDatosToSave++;
    }
}
//*************************************************************************************************
// Fin Metodo RecibirBytes de un muestreo
//*************************************************************************************************

//*************************************************************************************************
// Metodo para guardar los datos en el archivo, recibe el vector con los datos y el numero de datos
//*************************************************************************************************
void GuardarDatosEnArchivo(unsigned char *vectorDatosGuardar, unsigned int numDatosGuardar)
{
    // Tiempo de inicio del envio
    //    auto start = chrono::high_resolution_clock::now();
    // Variable que almacena los datos que se han guardado

    cout << "Guardando... " << endl;
    unsigned int datosGuardadosFwrite;
    // Vector para almacenar YY, MM, DD
    //    unsigned char tiempoLocal[3];

    //    cout << "Num Datos a Guardar" << to_string(numDatosGuardar) << endl;

    // Analiza si hay que crear un nuevo archivo o no
    if (isCrearNuevoArchivo == true)
    {
        // Reinicia la bandera
        isCrearNuevoArchivo = false;

        // En caso de que ya exista, cierra el archivo anterior
        if (objFile != NULL)
        {
            fclose(objFile);
        }

        // Actualiza la fecha anterior
        fechaAnteriorComp = fechaLongDSPIC;

        cout << "Fecha Long " << fechaLongDSPIC << endl;

        // Aqui se agrega la fecha al vector de datos. A la final la fecha solo sirve como cabecera
        // Guarda desde el MSB al LSB
        /*        ptrVectorDatosToSave[0] = (fechaLongDSPIC >> 24) & (0X000000FF);
        ptrVectorDatosToSave[1] = (fechaLongDSPIC >> 16) & (0X000000FF);
        ptrVectorDatosToSave[2] = (fechaLongDSPIC >> 8) & (0X000000FF);
        ptrVectorDatosToSave[3] = (fechaLongDSPIC) & (0X000000FF);
*/
        /*        vectorBytesTimeDSPIC[0] = (fechaLongDSPIC >> 24) & (0X000000FF);
        vectorBytesTimeDSPIC[1] = (fechaLongDSPIC >> 16) & (0X000000FF);
        vectorBytesTimeDSPIC[2] = (fechaLongDSPIC >> 8) & (0X000000FF);
        vectorBytesTimeDSPIC[3] = (fechaLongDSPIC) & (0X000000FF);
*/
        // Obtiene el nombre del archivo en funcion de la hora y fecha del dsPIC
        nombreArchivo = to_string(fechaLongDSPIC);
        nombreArchivo.append(PasarHoraLongToString(horaLongDSPIC));
        // Crea una variable con el nombre del archivo completo
        string archivoCompleto = path + nombreArchivo + extFile;
        // Concatena el path con el nombre de archivo y la extension
        cout << "Creacion de nuevo archivo: " << archivoCompleto << endl;
        // Abre o crea un archivo binario para guardar los datos muestreados
        // a es append: para agregar datos al archivo sin sobre escribir y en caso de que no haya el archivo lo crea
        // b es de binario. Se pasa el string.c_str() porque requiere una entrada cons char* (C-style string)
        objFile = fopen(archivoCompleto.c_str(), "ab");
    }

    if (objFile != NULL)
    {
        // Bucle hasta que se guarden todos los bytes
        do
        {
            // Guarda el vector de datos, siempre deben ser tipo char (ese es el tamaño configurado)
            datosGuardadosFwrite = fwrite(vectorDatosGuardar, sizeof(char), numDatosGuardar, objFile);
            //            cout << "Datos guardados " << to_string(datosGuardadosFwrite) << endl;
        } while (datosGuardadosFwrite != numDatosGuardar);
        // Flush
        fflush(objFile);
    }

    // Obtiene el tiempo que ha pasado desde el inicio del metodo
    //    auto elapsed = chrono::high_resolution_clock::now() - start;
    //    long long microseconds = chrono::duration_cast < chrono::microseconds > (elapsed).count();
    //    cout << "Tiempo archivo en us: " << to_string(microseconds) << endl << endl;
}
//*************************************************************************************************
// Fin Metodo para guardar los datos en el archivo, recibe el vector con los datos y el numero de datos
//*************************************************************************************************

//*************************************************************************************************
// Metodo para guardar los datos temporales de los 3 canales
//*************************************************************************************************
void GuardarDatosCanal(unsigned int valCH1, unsigned int valCH2, unsigned int valCH3)
{

    unsigned int vectorDatosCanal[3];
    unsigned int numDatosGuardados;

    vectorDatosCanal[0] = valCH1;
    vectorDatosCanal[1] = valCH2;
    vectorDatosCanal[2] = valCH3;

    fTmpCanal = fopen("/home/rsa/TMP/temporalCanal.txt", "w");

    if (fTmpCanal != NULL)
    {
        // Bucle hasta que se guarden todos los bytes
        do
        {
            // Guarda el vector de datos, siempre deben ser tipo char (ese es el tamaño configurado)
            numDatosGuardados = fwrite(vectorDatosCanal, sizeof(int), 3, fTmpCanal);
            //            cout << "Datos guardados " << to_string(datosGuardadosFwrite) << endl;
        } while (numDatosGuardados != 3);
        // Flush
        fflush(fTmpCanal);
    }

    fclose(fTmpCanal);

}
//*************************************************************************************************
// Fin Metodo para guardar los datos temporales de los 3 canales
//*************************************************************************************************

//*************************************************************************************************
// Metodo para indicar al dsPIC que comience el muestreo
//*************************************************************************************************
void IniciarMuestreo()
{
    cout << "Iniciando el muestreo..." << endl;
    // Envia el inicio de la trama para comienzo del muestreo
    bcm2835_spi_transfer(INI_INIT_MUES);
    bcm2835_delayMicroseconds(TIEMPO_SPI);
    // Envia el fin de la trama para comienzo del muestreo
    bcm2835_spi_transfer(FIN_INIT_MUES);
    bcm2835_delayMicroseconds(TIEMPO_SPI);

    // Activa la bandera de creacion de nuevo archivo
    isCrearNuevoArchivo = true;
}
//*************************************************************************************************
// Fin Metodo para indicar al dsPIC que comience el muestreo
//*************************************************************************************************

//*************************************************************************************************
// Metodo para obtener el tiempo del dsPIC
//*************************************************************************************************
bool ObtenerTiempoDSPIC()
{
    // Variables que se utilizan en el metodo
    unsigned char indiceForOTD, fuenteTiempoDSPIC, dataRecSPI;
    // Puntero para obtener la direccion de las dos variables de tiempo y guardar los bytes recibidos
    unsigned long *punteroLong;
    // Vector para almacenar los bytes de las dos variables long de tiempo
    //    unsigned char vectorBytesTimeDSPIC[8];
    // Valor que se retorna
    bool dataReturn = false;

    cout << endl
         << "Hora dsPIC: " << endl;
    // Envia el delimitador de inicio de trama para recibir el tiempo del dsPIC
    bcm2835_spi_transfer(INI_TIME_FROM_DSPIC);
    bcm2835_delayMicroseconds(TIEMPO_SPI);

    // Recibe el byte que indica la fuente de tiempo del PIC
    // 0 es del RTC y 1 es del GPS
    fuenteTiempoDSPIC = bcm2835_spi_transfer(DUMMY_BYTE);
    bcm2835_delayMicroseconds(TIEMPO_SPI);

    // Guarda los bytes de la hora y fecha tipo long devuelta por el dsPIC
    for (indiceForOTD = 0; indiceForOTD < 8; indiceForOTD++)
    {
        dataRecSPI = bcm2835_spi_transfer(DUMMY_BYTE);
        vectorBytesTimeDSPIC[indiceForOTD] = dataRecSPI;
        bcm2835_delayMicroseconds(TIEMPO_SPI);
    }

    // Envia el delimitador de final de trama para recibir el tiempo del dsPIC
    bcm2835_spi_transfer(FIN_TIME_FROM_DSPIC);
    bcm2835_delayMicroseconds(TIEMPO_SPI);

    if (fuenteTiempoDSPIC == FUENTE_TIME_RTC)
    {
        dataReturn = true;
        cout << "RTC " << endl;
    }
    else if (fuenteTiempoDSPIC == FUENTE_TIME_GPS)
    {
        dataReturn = true;
        cout << "GPS " << endl;
    }

    // Pasa los 4 bytes de la fecha a una sola variable tipo long, se desplazan los bytes a las posiciones
    // respectivas, primero va el byte MSB (desplazado 32 bits)
    fechaLongDSPIC = (vectorBytesTimeDSPIC[0] << 24) | (vectorBytesTimeDSPIC[1] << 16) | (vectorBytesTimeDSPIC[2] << 8) | vectorBytesTimeDSPIC[3];
    cout << "Fecha Long " << to_string(fechaLongDSPIC) << endl;
    // Lo mismo para la hora
    horaLongDSPIC = (vectorBytesTimeDSPIC[4] << 24) | (vectorBytesTimeDSPIC[5] << 16) | (vectorBytesTimeDSPIC[6] << 8) | vectorBytesTimeDSPIC[7];
    cout << "Hora Long " << to_string(horaLongDSPIC) << endl;

    // Llama al metodo para pasar el tiempo de las dos variables long a un vector
    // En el formato AA MM DD hh mm ss y se guarda en el vector
    PasarTiempoToVector(horaLongDSPIC, fechaLongDSPIC, vectorTimeDSPIC);

    // Muestra la fecha en formato YY-MM-DD
    cout << "Fecha YY-MM-DD " << to_string(vectorTimeDSPIC[0]) << '-' << to_string(vectorTimeDSPIC[1]) << '-' << to_string(vectorTimeDSPIC[2]) << endl;
    // Muestra la hora en formato hh:mm:ss
    cout << "Hora hh:mm:ss " << to_string(vectorTimeDSPIC[3]) << ':' << to_string(vectorTimeDSPIC[4]) << ':' << to_string(vectorTimeDSPIC[5]) << endl;

    /*
    // Aqui se agrega la fecha al vector de datos. Esto ocurre la primera vez, justo cuando comienza el muestreo
    ptrVectorDatosToSave[0] = (fechaLongDSPIC >> 24) & (0X000000FF);
    ptrVectorDatosToSave[1] = (fechaLongDSPIC >> 16) & (0X000000FF);
    ptrVectorDatosToSave[2] = (fechaLongDSPIC >> 8) & (0X000000FF);
    ptrVectorDatosToSave[3] = (fechaLongDSPIC) & (0X000000FF);
    // Por ultimo, en cuanto a la hora, siempre el MSB va a ser 0, entonces se guarda este valor
    ptrVectorDatosToSave[4] = 0;
    // El resto de datos se recibe desde el dsPIC
    ptrVectorDatosToSave[5] = (horaLongDSPIC >> 16) & (0X000000FF);
    ptrVectorDatosToSave[6] = (horaLongDSPIC >> 8) & (0X000000FF);
    ptrVectorDatosToSave[7] = (horaLongDSPIC) & (0X000000FF);
*/

    return dataReturn;
}
//*************************************************************************************************
// Fin Metodo para obtener el tiempo del dsPIC
//*************************************************************************************************

//******************************************************************************
// Funcion para pasar la hora y la fecha en tipo long a un vector con los indices
// AA MM DD hh mm ss
// Recibe la hora y la fecha en una sola variable tipo long y tambien recibe el
// vector en el que se va a pasar la fecha y hora en 6 posiciones
//******************************************************************************
void PasarTiempoToVector(unsigned long longHora, unsigned long longFecha, unsigned char *tramaTiempoSistema)
{
    // Variables para almacenar los 6 valores de tiempo
    unsigned char anio, mes, dia, hora, minuto, segundo;

    // Obtiene las horas, minutos y segundos
    hora = longHora / 3600;
    minuto = (longHora % 3600) / 60;
    segundo = (longHora % 3600) % 60;

    // Obtiene el año, mes y dia
    anio = longFecha / 10000;
    mes = (longFecha % 10000) / 100;
    dia = (longFecha % 10000) % 100;

    // Guarda en el vector los datos de tiempo
    tramaTiempoSistema[0] = anio;
    tramaTiempoSistema[1] = mes;
    tramaTiempoSistema[2] = dia;
    tramaTiempoSistema[3] = hora;
    tramaTiempoSistema[4] = minuto;
    tramaTiempoSistema[5] = segundo;
}
//******************************************************************************
//******************** Fin Metodo PasarTiempoToVector **************************
//******************************************************************************

//******************************************************************************
// Funcion para pasar la hora tipo long a una variable tipo string en el formato
// "hhmmss". Recibe una variable long con la hora en segundos y devuelve un string
//******************************************************************************
string PasarHoraLongToString(unsigned long longHora)
{
    // Variables para almacenar los 3 valores de tiempo
    unsigned char hora, minuto, segundo;
    // Variable tipo string para almacenar la hora en formato hhmmss
    string horaStr = "000000";

    // Obtiene las horas, minutos y segundos
    hora = char(longHora / 3600);
    minuto = char((longHora % 3600) / 60);
    segundo = char((longHora % 3600) % 60);

    // Analiza si los valores de hora, minuto y segundo tienen solo unidades
    // En ese caso hay que añadir un 0 a la izquierda para mantener el formato de hhmmss
    // Pasa a string la hora. Si las horas son menores a 10 hay que agregar un 0 a la izquierda
    if (hora >= 10)
    {
        horaStr = to_string(hora);
    }
    else
    {
        horaStr = string(1, '0').append(to_string(hora));
    }
    // De la misma manera para los minutos
    if (minuto >= 10)
    {
        horaStr = horaStr + to_string(minuto);
    }
    else
    {
        horaStr = horaStr + string(1, '0').append(to_string(minuto));
    }
    // Por ultimo los segundos
    if (segundo >= 10)
    {
        horaStr = horaStr + to_string(segundo);
    }
    else
    {
        horaStr = horaStr + string(1, '0').append(to_string(segundo));
    }

    return horaStr;
}
//******************************************************************************
//******************** Fin Metodo PasarHoraLongToString **************************
//******************************************************************************

//*************************************************************************************************
// Metodo para obtener el tiempo actual de la RPi y enviarlo al micro
//*************************************************************************************************
void EnviarTiempoLocal()
{
    // Vector para almacenar los datos de tiempo YY MM DD hh mm ss
    unsigned char tiempoLocal[6];
    unsigned short indiceFor;
    // Variables tipo long para almacenar la fecha completa y la hora en segundos
    unsigned long horaLong, fechaLong;
    // Vector para almacenar los bytes de las dos variables tipo long
    unsigned char vectoBytesTime[8];

    // Obtiene la hora y la fecha del sistema
    cout << endl
         << "Envio de Hora local: " << endl;
    time_t t;
    struct tm *tm;
    t = time(NULL);
    tm = localtime(&t);
    tiempoLocal[0] = tm->tm_year - 100; // Anio (contado desde 1900)
    tiempoLocal[1] = tm->tm_mon + 1;    // Mes desde Enero (0-11)
    tiempoLocal[2] = tm->tm_mday;       // Dia del mes (0-31)
    tiempoLocal[3] = tm->tm_hour;       // Hora
    tiempoLocal[4] = tm->tm_min;        // Minuto
    tiempoLocal[5] = tm->tm_sec;        // Segundo

    // Muestra la fecha en formato YY-MM-DD
    cout << "Fecha YY-MM-DD " << to_string(tiempoLocal[0]) << '-' << to_string(tiempoLocal[1]) << '-' << to_string(tiempoLocal[2]) << endl;
    // Muestra la hora en formato hh:mm:ss
    cout << "Hora hh:mm:ss " << to_string(tiempoLocal[3]) << ':' << to_string(tiempoLocal[4]) << ':' << to_string(tiempoLocal[5]) << endl;

    // Pasa el tiempo a dos variables tipo long de fecha y hora en segundos
    // Calcula el segundo actual = hh*3600 + mm*60 + ss
    horaLong = (tiempoLocal[3] * 3600) + (tiempoLocal[4] * 60) + (tiempoLocal[5]);
    cout << "Hora Long " << horaLong << endl;
    // Calcula la fecha actual completa = 10000*AA + 100*MM + DD
    fechaLong = (tiempoLocal[0] * 10000) + (tiempoLocal[1] * 100) + (tiempoLocal[2]);
    cout << "Fecha Long " << fechaLong << endl;

    // Obtiene los bytes de cada variable y los almacena en el vector
    // Separa los bytes de la fecha, desde MSB a LSB
    vectoBytesTime[0] = char(fechaLong >> 24);
    vectoBytesTime[1] = char(fechaLong >> 16);
    vectoBytesTime[2] = char(fechaLong >> 8);
    vectoBytesTime[3] = char(fechaLong);
    // De forma similar para la hora
    vectoBytesTime[4] = char(horaLong >> 24);
    vectoBytesTime[5] = char(horaLong >> 16);
    vectoBytesTime[6] = char(horaLong >> 8);
    vectoBytesTime[7] = char(horaLong);

    // Tiempo de inicio del envio
    auto start = chrono::high_resolution_clock::now();

    // Envia el delimitador de inicio de trama para envio de tiempo desde la RPi
    bcm2835_spi_transfer(INI_TIME_FROM_RPI);
    bcm2835_delayMicroseconds(TIEMPO_SPI);
    // Envia los 8 bytes del vector de tiempo al dsPIC
    for (indiceFor = 0; indiceFor < 8; indiceFor++)
    {
        bcm2835_spi_transfer(vectoBytesTime[indiceFor]);
        bcm2835_delayMicroseconds(TIEMPO_SPI);
    }
    // Envia el delimitador de final de trama para envio de tiempo desde la RPi
    bcm2835_spi_transfer(FIN_TIME_FROM_RPI);
    bcm2835_delayMicroseconds(TIEMPO_SPI);

    // Obtiene el tiempo que ha pasado desde el inicio del metodo
    auto elapsed = chrono::high_resolution_clock::now() - start;
    long long microseconds = chrono::duration_cast<chrono::microseconds>(elapsed).count();
    cout << "Tiempo de envio en us: " << to_string(microseconds) << endl;
}
//*************************************************************************************************
// Fin Metodo EnviarTiempoLocal
//*************************************************************************************************

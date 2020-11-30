#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <openssl/md5.h> // Para calcular el MD5
#include <sys/socket.h> // Para crear sockets, enviar, recibir, etc
#include <netdb.h> // Para getaddrinfo
#include <unistd.h> // Para close
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include "../../Utils/includes/protocolo.h"
#ifndef CONSOLA_H_
#define CONSOLA_H_

    //Prototipos De funcion
    void * consolaPlan(void * unused);

    //Estructura de los comandos.
    typedef struct {
        char *name;			//Nombre de la funcion para el usuario.
        char *doc;			//Cuál es su funcionalidad.
    } COMMAND;

    //Array de comandos de la consola.
    
    void convertir(char *palabra);

    void com_Pause();

    void com_Continue();

    void com_Kill(int ID);

    void com_Lock(char *clave, int ID);

    void com_Unlock(char *clave);

    void com_Deadlock();

    void com_Status(char *clave);

    void com_List(char *recurso);

    void com_Imprimir();

    void GOODBYE_WORLD();

#endif /* CONSOLA_H_ */
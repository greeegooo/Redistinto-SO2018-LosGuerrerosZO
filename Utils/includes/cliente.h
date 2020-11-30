#ifndef CLIENTE_H
#define CLIENTE_H

/********************************************************INCLUDES***/
/*******************************************************************/
#include <stdio.h>
#include <openssl/md5.h> // Para calcular el MD5
#include <string.h>
#include <stdlib.h> // Para malloc
#include <sys/socket.h> // Para crear sockets, enviar, recibir, etc
#include <netdb.h> // Para getaddrinfo
#include <unistd.h> // Para close
#include <readline/readline.h> // Para usar readline
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include "protocolo.h"

/*******************************************************CONSTANTS***/
/*******************************************************************/


/*********************************************************GLOBALS***/
/*******************************************************************/

/*********************************************************STRUCTS***/
/*******************************************************************/


/******************************************************PROTOTYPES***/
/*******************************************************************/

void exit_gracefully(t_log * logger, int return_nr);

void _exit_with_error(t_log * logger, int socket, char* error_msg, void * buffer);

#endif /* CLIENTE_H_ */

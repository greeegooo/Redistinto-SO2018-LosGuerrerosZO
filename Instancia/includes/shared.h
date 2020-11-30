#ifndef SHARED_H_
#define SHARED_H_

#include <stdio.h>
#include <stdlib.h> // Para malloc
#include <string.h> // Para memset
#include <sys/socket.h> // Para crear sockets, enviar, recibir, etc
#include <netdb.h> // Para getaddrinfo
#include <pthread.h>
#include <unistd.h> // Para close
#include <errno.h> 
#include <arpa/inet.h>    //close 
#include <math.h>
#include "../../Utils/includes/protocolo.h"
#include "../../Utils/includes/commons.h"
#include "../../Utils/includes/communications.h"


extern t_log * logger;

#endif 
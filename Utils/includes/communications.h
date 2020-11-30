#ifndef COMMUNICATIONS_H
#define COMMUNICATIONS_H

#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include "cliente.h"

int connect_to_server(t_log * logger, char * ip, char * port);

#endif
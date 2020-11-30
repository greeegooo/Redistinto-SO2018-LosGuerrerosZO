#ifndef RUN_H_
#define RUN_H_

#include "shared.h"
#include "instancia.h"
#include "entrada.h"
#include "aux.h"

#define TOKEN_FIN_TEXTO ""
#define EXTENSION_ARCHIVO ".txt"
#define TAMANIO_MAX_VALOR 255

char * clave_aux_existeClave;
char * clave_aux_compactar;
char * clave_aux_existeClave_Compactar;

Instancia * instanciaAux;

void run(Instancia ** instancia);

bool escribir_en_disco(char * path, char * valor);

char * obtener_path(Instancia ** instancia, Sentencia * sentencia);

char * obtener_path_dump(Instancia ** instancia, char * clave);

char * obtener_valor_tabla(Instancia ** instancia, Sentencia * sentencia);

char * obtener_valor_archivo(Instancia ** instancia, Sentencia * sentencia);

void Levantar_Archivo(Instancia ** instancia, Sentencia * sentencia);

void Dump(Instancia * instancia);



#endif 
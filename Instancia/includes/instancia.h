#ifndef INSTANCIA_H
#define INSTANCIA_H

/********************************************************INCLUDES***/
/*******************************************************************/
#include "shared.h"

/*******************************************************CONSTANTS***/
/*******************************************************************/
#define CLIENTE_DESCONECTADO 0
#define ERROR_RECV -1
#define NO_EXISTE -1

/*********************************************************GLOBALS***/
/*******************************************************************/

/*********************************************************STRUCTS***/
/*******************************************************************/

typedef enum {
	CIRC,
	LRU,
	BSU
}ALG_REEMPLAZO;

struct instancia{
  	int id;
  	int socket;
	int cantidad_Entradas;
	int tamanio_Entrada;
	t_list * tabla_Entradas;
	char * memoria;
	int tamanio_Memoria;
	char * montaje;
	int intervalo;
	ALG_REEMPLAZO algoritmo;
};
typedef struct instancia Instancia;

typedef struct {
	int CantEntradas;
	int TamEntradas;
}__attribute__((packed)) sizeInstancia; 

/******************************************************PROTOTYPES***/
/*******************************************************************/

// Constructor
Instancia * crear_instancia();

// Destructor
void liberar_instancia(Instancia **instancia);

#endif
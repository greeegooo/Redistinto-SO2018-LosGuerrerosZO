#ifndef ENTRADA_H_
#define ENTRADA_H_

/********************************************************INCLUDES***/
/*******************************************************************/
#include "shared.h"
#include "instancia.h"

/*********************************************************STRUCTS***/
/*******************************************************************/

struct entrada{
	//ID de Entrada
	int id;
	//Clave asociada a la entrada
	char clave[255];
	//Tamanio de la entrada
	int tamanio;
	//Bool para saber si la entrada esta ocupada
	bool ocupado;
	//Bool para saber si la entrada es atomica
	bool esAtomico;
	//Puntero al inicio de la entrada
	char * ptrEntrada;
	//Bool para saber si es la victima para reemplazar por CIRC
	bool esVictima;
	//Indica hace cuantas sentencias fue referenciado
	int referenciado;
	//Cantidad de bytes ocupados en la entrada
	int bytesOcupados;
	//Cantidad total de entradas ocupadas
	int entradasOcupadas;
	//Tamanio neto del valor
	int tamanioValor;
}; 
typedef struct entrada t_entrada;

/******************************************************PROTOTYPES***/
/*******************************************************************/

t_entrada * Crear_Entrada(int id, int tamanio, void * ptrEntrada);
void Liberar_Entrada(t_entrada ** entrada);
int Entradas_necesarias(char * valor, int tamanio);
bool Es_entrada_libre(t_entrada * entrada);
t_entrada * Obtener_entrada(t_list * tabla, int index);
bool Hay_entradas_libres(Instancia ** instancia);

t_link_element * ObtenerMemoriaContiguaLibrePara(Instancia ** instancia, Sentencia * sentencia);
RESULTADO Escribir(t_link_element * entrada, Sentencia * sentencia);
RESULTADO Reemplazar(Instancia ** instancia, Sentencia * sentencia);
void Compactar(Instancia ** instancia);

bool _es_libre(void * entrada);

#endif 
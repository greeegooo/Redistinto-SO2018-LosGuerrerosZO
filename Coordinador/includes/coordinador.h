/******************************************************************************/
/*							     DEPENDENCIAS				                  */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h> 
#include <pthread.h> 
#include <ctype.h> 
#include <math.h>
#include "../../Utils/includes/protocolo.h"
#include "../../Utils/includes/cliente.h"
#include "../../Utils/includes/commons.h"
#include "../../ESI/includes/esi.h"

#ifndef COORDINADOR_H
#define COORDINADOR_H
#define PLANIFICADOR 1
#define INSTANCIA 2
#define ESI 3
#define CLAVES 4
#define DISTRIBUCION 4
#define INSTANCIA_NOT_DIE -1
#define ESI_NOT_DIE -1
#define PLANIFICADOR_NOT_DIE -1
#define TODAS_DESPACHADAS -1
#define LETRA_A 97
#define LETRA_Z 122
#define REDISTRIBUCION 3
#define LETRAS_TOTALES 25
#define INSTANCIA_EXISTENTE -1
#define INSTANCIA_EXISTIA -2
#define INSTANCIA_NO_EXISTE 0
#define EQUITATIVE 0
#define DESEMPATE 1
#define CLAVE_INEXISTENTE -1
#define CON_EFECTO 2
#define SIMULACION 4
#define TODOS_MENOS_ID 1
#define TODOS_MENOS_INSTANCIAS 2
#define LOCK 1
#define UNLOCK 2


/******************************************************************************/
/*							DEFINICIONES Y DECLARACIONES				      */
/******************************************************************************/

	//Esto deberia ir por archivo de configuración (creo)

	t_dictionary * diccionarioRecursosPedidos;
	bool Coordinar = true;
	
	
/******************************************************************************/
/*							FUNCIONES PRIVADAS								  */
/******************************************************************************/
	typedef struct{
	int id;
	int Socket;
	pthread_mutex_t Semaforo;
	int Valor;
	}AdmCliente; 
	
	typedef struct{
		char Clave[255];
		int IdInstancia;
		int EntradasQueOcupa;
	}Clave_Instancia;

	typedef struct {
		int CantEntradas;
		int TamEntradas;
	} __attribute__((packed)) sizeInstancia;
	
	typedef enum {
        LSU,
        EL,
		KE,
    } ALGORITMOS_DISTRIBUCION;

	AdmCliente* Crear_AdmCliente(int socket_cli, int id_cliente);
	void Crear_Diccionario();
	void Abir_Puerto();
	void Despachar_Pedido(Payload * pedido);
	void Eliminar_Diccionario();
 	void Bloquear_ESI(int id);
	void Comprimir();
	void Solicitar_Recurso(Payload * pedido);
	void Escribir_Recurso(Payload * pedido);
	void Liberar_Recurso(Payload * pedido);
	Payload * Escuchar_Planificador();
	void Abortar_ESI(int idESI);
	bool Recurso_Bloqueado(char * clave);
	int Id_Reservo_Recurso(char * clave);

#endif /* COORDINADOR_H */

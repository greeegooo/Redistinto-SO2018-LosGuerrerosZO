/*
 * Protocolo.h
 *
 *  Created on: 21 abr. 2018
 *      Author: pfilice
 */

#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

	#include <stdlib.h>
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

	#define SIN_ID_SOLICITANTE 0
	#define ID_PLANIFICADOR -2
	#define ID_COORDINADOR -1
	#define SIN_RESULTADO 0
	#define SIN_TAMANIO 0
	#define SIN_PAYLOAD 0
	#define SIN_DATOS NULL
	#define RECURSO_DISPONIBLE 1
	#define RECURSO_TOMADO 0
	#define SIN_ELEMENTOS 0
	//#define OK 1
	//#define BLOQUEAR 2
	//#define ABORTAR 3

	typedef enum {
		OK,
		BLOQUEAR,
		ABORTAR
	} RESULTADO;

	typedef enum {
		//Asuntos del Coordinador al Planificador
		SENTENCIA_PLANIFICADOR_REQ,
		//Asuntos del Planificador al Coordinador
		SENTENCIA_PLANIFICADOR_RESP,
		HANDSHAKE_PLAN,
		HANDSHAKE_PLAN_COORD,
		VALOR_CLAVE,
		POSIBLE_INSTANCIA,
		//Asuntos del ESI al Coordinador
		EJECUTAR_SENTENCIA_REQ,
		HANDSHAKE_ESI_COORD,
		//Asuntos del Coordinador al ESI
		HANDSHAKE_COORD_ESI,
		EJECUTAR_SENTENCIA_RESP,
		//Asuntos del ESI al Planificador
		NUEVO_ESI,
		ESI_OK,
		ESI_BLOQUEO,
		ESI_ABORTO,
		//Asuntos del Planificador al ESI
		ASIGNACION_PID,
		EJECUTATE,
		//Asuntos del Coordinador a Instancias
		SENTENCIA_INSTANCIA_REQ,
		HANDSHAKE_COORD_INST,
		CLAVES_EXISTENTES,
		CMD_STATUS,
		//Asuntos de Instancias al Coordinador
		SENTENCIA_INSTANCIA_RESP,
		COMPACTAR,
		COMPACTAR_OK,
		//Asuntos de Planificador a ESI por consola
		ABORTAR_ESI,
		ESI_PLAN_ABORTE,
		BLOQUEAR_ESI,	
		//COORD A ESI O ESI A PLANI
		//EJECUTAR_SENTENCIA, //ESI a Coordinador
		//CONFIRMAR_SENTENCIA, //Coordinador a Planificador
		//RESPUESTA_CONFIRMAR_SENTENCIA, //Planificador a Coordinador
		
		SENTENCIA_EJECUTADA, //Coordinador a Planificador
		RECURSO_LIBERADO, //Coordinador a Planificador
		CONSULTA_RECURSO_DISPONIBLE,
		ESI_PLAN_TERMINE,//ESI A PLANI
		SENTENCIA_EJECUTADA_CON_ERROR,
		SENTENCIA_EJECUTADA_CON_EXITO,
		HANDSHAKE_ESI,
		HANDSHAKE_COORD,//COORD A CLIENTES
		//RECURSO_DISPONIBLE, //PLANI A COORD
		//RECURSO_TOMADO // PLANI COORD
		PLANIFICADOR_SET_RESP,
		PLANIFICADOR_GET_RESP,
		PLANIFICADOR_STORE_RESP,
		INSTANCIA_GET_RESP,
		INSTANCIA_SET_RESP,
		INSTANCIA_STORE_RESP,
		HANDSHAKE_ESI_PLAN,
		//RECURSO_DISPONIBLE, //PLANI A COORD
		//RECURSO_TOMADO // PLANI COORD
		OTRO_ASUNTO,
		CLAVES_EXISTENTES_OK,
		CLAVES_EXISTENTES_ABORTAR
	} ASUNTO;

	typedef struct {
		ASUNTO Asunto;
		int Length;
		int IdSolicitante;
		int Identificador_Transaccion;
	} __attribute__((packed)) Header;

	typedef enum {
		S_GET,
		S_SET,
		S_STORE
	} Acciones_Operacion;

	typedef void * Payload;

	typedef struct{
		Acciones_Operacion Operacion;
		char Clave[255];
		char Valor[255];
		RESULTADO Resultado;
	} __attribute__((packed)) Sentencia;

	Header * Crear_Header_Vacio();
	
	/**
	* @NAME: Crear_Header
	* @DESC: Recibe un id de operacion o resultado y un tamaño para el campo data del payload (carga útil)
	*		 y devuelve una cabecera con esos datos asignados
	* @PARAMS:
	* 		id - Código que indica la operación o el resultado de un pedido
	* 		length - Tamaño que indica cuanta memoria reservar para los datos del payload
	*/
	Header * Crear_Header(ASUNTO asunto, int length, int idSolicitante);

	/**
	* @NAME: Destruir_Header
	* @DESC: Libera la memoria reservada para el Header
	* @PARAMS:
	* 		header
	*/
	void Destruir_Header(Header * header);

	/**
	* @NAME: Crear_Payload
	* @DESC: A traves de un header y unos datos a pasar se crea un payload
	* @PARAMS:
	* 		header - Con id y tamaño de los datos de payload seteados
	* 		data - Informacion sin discriminación de tipo
	*/
	Payload Crear_Payload(Header * header, void * data);	/**
	* @NAME: Destruir_Payload
	* @DESC: Libera la memoria reservada 	para el payload
	* @PARAMS:
	* 		payload
	*/
	void Destruir_Payload(Payload payload);

	int Tamanio_Payload(Header * header);

	void * Contenido(Payload payload);

	void *Inicializar_Sentencia();

	void Enviar_Sentencia(int cliente, Sentencia *sentencia, int opcion, int id);

	Sentencia* Recibir_Sentencia(int cliente);

	Sentencia * Crear_MockSentencia(Acciones_Operacion op, char * clave, char * valor);

	char *Traducir_Resultado(int resultado);
	
	char *Traducir_Operacion(int operacion);

#endif /* PROTOCOLO_H_ */
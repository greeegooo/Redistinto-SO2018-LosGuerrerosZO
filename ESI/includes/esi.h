#ifndef ESI_H
#define ESI_H

	#include <stdlib.h>
    #include <string.h>
    #include <commons/collections/queue.h>
    #include <stdio.h>
    #include <commons/log.h>
    #include <commons/config.h>
    #include <commons/collections/dictionary.h>
    #include <commons/collections/list.h>
    #include "../../Utils/includes/cliente.h"
    #include "../../Utils/includes/protocolo.h"
	#include "../../Utils/includes/communications.h"
    #include <parsi/parser.h>

	#define ARCHIVO_INVALIDO -1
	#define ERROR_CONEXION_COORDINADOR -2
	#define ERROR_CONEXION_PLANIFICADOR -3
	#define ESI_ABORTADO -4
	#define PID_SIN_ASIGNAR 0
	#define TAMANIO_MAXIMO_SENTENCIA 512
	#define TOKEN_FIN_SENTENCIA "\n"
	#define FIN_DE_CADENA ""

	typedef struct {
		char * Clave;
		char * Entrada;
		char * Valor;
		Acciones_Operacion Accion;
	} Operacion;

	typedef struct NodoOperacion {
		Operacion Datos;
		struct NodoOperacion * Siguiente;
	} NodoOperacion;

	typedef enum {
		LISTO,
		EJECUTANDO,
		BLOQUEADO,
		FINALIZADO,
		NUEVO,
		ABORTADO
	} Estados_ESI;

	typedef enum{
		FIN_SCRIPT,
		COORD_ABORTO,
		PLAN_ABORTO,
		SENTENCIA_INVALIDA
	} MOTIVO_FIN;

	typedef struct {
		char * Clave;
		int IdESI;
	} Recurso;

	char stringScript[100];

	FILE * fileScript;

	int cantidadSentenciasRecuperadas = 0;

	int numeroSentenciaEjecutar = 1;

	bool mockOn = false;

	int mockSentenciaAParsear = GET;

	char * sentenciasRecuperadas = NULL;

	t_esi_operacion operacionActual;

	void Destruir_Buffer(void * buffer);
	void Enviar_Mensaje(int socket, Header * header, void * data, t_log * logger);
	int Recibir_Pid(int socket, int length, t_log * logger);
	void Hello_Coordinador(int socket_Coordinador, int pid, t_log * logger);
	int Handshake_Planificador(int socket_Planificador, t_log * logger);
	void Recibir_Mensajes(int socket_Planificador, int socket_Coordinador, int pid, t_log * logger);
	char * Nombre_Operacion_Actual();
	Sentencia * Asignar_Sentencia();
	void Cargar_Script(t_log * logger);
	void Terminar_Ejecucion(int socket_Planificador, int pid, MOTIVO_FIN motivo, t_log * logger);
	void Abortar(MOTIVO_FIN motivo);
	void Evaluar_Sentencia_Response(Sentencia * sentencia, int socket_Planificador, int pid, t_log * logger);
	void Avanzar_Proxima_Sentencia(int socket_Planificador, int pid, t_log * logger);
	Sentencia * MOCK_Recibir_Sentencia();
	void Cargar_Script(t_log * logger);
	void Parsear_Sentencia(int socket_Planificador, int pid, t_log * logger);
	void MOCK_Parsear_Sentencia(int socket_Planificador, int pid, t_log * logger);

#endif
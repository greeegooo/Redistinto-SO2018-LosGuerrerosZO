#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_

    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <ctype.h>
    #include <readline/readline.h>
    #include <readline/history.h>
    #include <signal.h>
    #include <pthread.h>
    #include <openssl/md5.h> // Para calcular el MD5
    #include <sys/socket.h> // Para crear sockets, enviar, recibir, etc
    #include <netdb.h> // Para getaddrinfo
    #include <unistd.h> // Para close
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include "../../Utils/includes/commons.h"
    //#include <commons/log.h>
    //#include <commons/config.h>
    //#include <commons/collections/dictionary.h>
    //#include <commons/collections/list.h>
    #include "../../Utils/includes/protocolo.h"
    #include "../../Utils/includes/cliente.h"
    #include "../../Utils/includes/communications.h"
    #include <sys/time.h>
    #define FALLO_MULTIPLEXACION 4
    #define PIDSOCKET 10
    #define PIDRECURSO 11
    #define PIDBLOQUEADOS 12
    #define ESTRUCTURA_ESTANDAR 13
    #define SISTEMA -1

   typedef int PID;

    typedef enum {
        FIFO,
        SJF_CD,
        SJF_SD,
        HRRN
    } ALGORITMOS_PLANIFICACION;

    typedef struct {
        int Pid;
        char * Key;
    } Pid_Key;

    typedef struct {
        int Pid;
        int Socket;
    } Pid_Socket;

    typedef struct {
        int Pid;
        int Instante_Creacion;
        int Instante_Desbloqueo;
        int Rafagas_Efectuadas;
        float Tiempo_Espera;
        float Estimacion_ESI;
        float Ratio_Response;
    } ESI_Plan;

    typedef struct {
        bool ClienteDesconectado;
        int socket;
    } Estado_Cliente;
    
    typedef enum ESTADOS_PLANIFICACION {
        NUEVO,
        EJECUCION,
        EJECUCION_TERMINADA,
        FINALIZADO,
        BLOQUEADO,
        DESBLOQUEADO,
        ABORTO_EJECUCION
    } ESTADOS_PLANIFICACION;

    //Variables para multiplexar conexiones
    fd_set master;
    fd_set read_fds;

    ESTADOS_PLANIFICACION estado_Planificacion = NUEVO;

    bool activarPlanificacion = true;

    bool coordinadorInactivo = true;

    bool enEjecucion = false;

    int reloj = 0;

    int ultimo_Reloj_Ejecucion = -1;

    int ultimo_Reloj_ESI_Estimado = 0;

    int ultimo_Pid = 1;

    int tiempo_Trabajo_Minimo = INT16_MAX;

    float estimacion_Inicial_ESI;

    int pid_Ejecucion;

    pthread_mutex_t sem_ejecucion;

    int pid_Planificacion;

    int pid_Planificacion_Aux;

    int socket_Coordinador;

    t_log * logger;

    int servidor;

    char claveADesbloquear[40];

    char claveABloquear[40];      

    ALGORITMOS_PLANIFICACION algoritmo_Planificacion;

    int pid_a_borrar;

    bool me_mori;

    bool consolaCorriendo = false;

    int pid_A_Desbloquear;

    int pid_A_Bloquear;

    int pid_Socket_A_Buscar;

    int socket_A_Buscar;

    int pid_A_Buscar;

    float alfa_Planificacion;

    char claveRecurso[40];

    char DeadLock_Clave_Index[40];

    int Deadlock_Pid;

    char Deadlock_Clave_Match[40];

    bool planificadorPausado = false;

    typedef struct{
        int pid_inicial;
        int pid_remitente;
        int pid_destinatario;
    }mensaje_deadlock;

    int Deadlock_Pid_Inicial;

    t_list * lista_ESI_Planificacion; // ESI PLAN   
    #define PIDESIPLAN 18
    t_list * lista_Pid_Socket;       // PID - SOCKET
    #define PIDSOCKET 10
    t_list * lista_Pid_Recurso;      // PID - RECURSO
    #define PIDRECURSO 11
    t_list * lista_Pid_Bloqueados; // PID - REC BLOQ    ESPERO UN RECURSO
    #define PIDBLOQUEADOS 12
    t_list * lista_Pid_Listos;       // PID
    #define PIDLISTOS 13
    t_list * lista_Pid_Nuevos;      // PID
    #define PIDNUEVOS 14
    t_list * lista_Pid_Desbloqueados; // PID
    #define PIDDESBLOQUEADOS 15
    t_list * lista_Pid_Finalizados; // PID
    #define PIDFINALIZADOS 16
    t_list * lista_Pid_Ejecucion; // PID
    #define PIDEJECUCION 17

    int socket_Coordinador;
    
    bool Clave_A_Desbloquear(void * pid_Key);
    
    bool Clave_Disponible(char * clave, int pid);
    
    bool ES_ESI_Plan_Buscado_Por_Pid_A_Buscar(void * esi_Plan);
    
    bool Es_ESI_Plan_Creado_Mas_Antiguo_Multiplicidad_Eventos(void * esi_Plan1, void * esi_Plan2);
    
bool Es_Clave_Pid_Recurso_Por_Clave_Recurso (void * pid_Key);

bool Es_Clave_Pid_Recurso_Por_Pid_A_Buscar(void * pid_Key);

bool Es_Ejecucion_Valida();

    bool Es_ESI_Plan_Creado_Mas_Antiguo(void * esi_Plan1, void * esi_Plan2);
    
    bool Es_ESI_Plan_Desbloqueado_Mas_Antiguo(void * esi_Plan1, void * esi_Plan2);
    
    bool Es_ESI_Plan_Mayor_Prioridad_Multiplicadad_Eventos(void * esi_Plan1, void * esi_Plan2);
    
    bool Es_ESI_Plan_Menor_Tiempo_Multiplicadad_Eventos(void * esi_Plan1, void * esi_Plan2);
    
    bool Es_ESI_Plan_Menor_Tiempo(void * esi_Plan1, void * esi_Plan2);
    
    bool Es_ESI_Planificacion_Por_Pid_Planificacion_Aux(void * esi_Plan);
    
    bool Es_ESI_Planificacion_Por_Pid_Planificacion(void * esi_Plan);
    
    bool Es_Socket_Pid_Por_Pid_Ejecucion(void * socket_Pid);

    bool Es_Pid_Buscado_Por_Pid_Planificacion_Aux(void * pid);
    
    bool Es_Pid_Buscado_Por_Pid_Planificacion(void * pid);
    
    bool Es_Pid_Buscado(void * pid);
    
    bool Es_Pid_Plan_A_Borrar(void * esi_Plan);
    
    bool Es_Pid_Socket_Buscado(void * pid_Socket);
    
    bool Es_Socket_ESI_En_Ejecucion_Por_Socket_A_Buscar(void * pid_Socket);
    
    bool Es_Socket_Pid_Ejecucion(void * pid);
    
    bool Se_Encuentra_ESI_Plan_Por_Pid_A_Buscar(void * esi_Plan);
    
    bool Se_Encuentra_Pid_A_Borrar_Por_Pid_Key(void * pid_Key);
    
    bool Validar_Posesion_Key(int pid, Sentencia * sentencia);
    
    Header * Recibir_Header(int socket);

    int Encontrar_Socket_Pid_Ejecucion();
    
    void Esperar_Fin_Ejecucion();

    void * list_remove_first(t_list * lista);
    
    void * Recibir_Datos(int socket, int tamanioDatos);
    
    void Abortar_ESI_Ejecucion(bool comunicar);
    
    void Actualizar_Datos_ESI_Ejecucion();
    
    void Actualizar_Pid();
    
    void Actualizar_Reloj();
    
    void Eliminar_De_Listas_Menos_Bloqueado(int pid);

    void Actualizar_Tiempos_Espera_ESI();
    
    void Actualizar_Tiempos_Espera_ESI();
    
    void Actualizar_Tiempos_Espera_ESI();
    
    void Bloquear_ESI_Ejecucion(char * clave);
    
    void Calcular_Ratio_Response_ESI(ESI_Plan * esi_Plan);
    
    void Cargar_Algoritmo(char * string_algoritmo_Planificacion);
    
    void com_Continue();
    
    void com_Deadlock();
    
    void com_Imprimir();
    
    void com_Kill(int ID);
    
    void com_List(char *recurso);
    
    void com_Lock(char *clave, int ID);
    
    void com_Pause();
    
    void com_Status(char *clave);
    
    void com_Unlock(char *clave);
    
    void Destruir_Pid_Key(void * pid_Key);
    
    void Ejecutar_ESI_Listo();
    
    void Eliminar_Elemento_Lista(int index, int opcion);
    
    void Eliminar_PID_Lista(t_list *list, int pid, int opcion);
    
    void Eliminar_Socket_Ejecucion(int socket);
    
    void Eliminar_Socket(int pid);
    
    void Estimar_Tiempo_ESI(int pid);
    
    void Finalizar_ESI_Ejecucion();
    
    void Finalizar_ESI(int pid);
    
    void GOODBYE_WORLD();
    
    void Inicializar_Lista_Pid_Recurso(t_list * lista, char ** claves);
    
    void Liberar_Clave(char * clave);
    
    void Liberar_ESIS_Bloqueados_Por_Clave(char * clave);
    
    void Liberar_Todos_Los_Datos(int pid);
    
    void Mandar_Ejecutate_ESI_Ejecucion();
    
    void Pasar_ESI_Finalizado(int pid);
    
    void Planificar_FIFO();
    
    void Planificar_HRRN();
    
    void Planificar_SJF_CD();
    
    void Planificar_SJF_SD();
    
    void Send_Liberar_Clave_Coord(char * clave);
    
    void Send_Payload(int socket, ASUNTO asunto, int tamanioDatos, void * datos);

    void Imprimir_Lista_ESI_PLAN();

    /*****************************************************************************
                            
                            PROTOTIPOS DE FUNCION DE LA CONSOLA
    **************************************************************************/
    
    void com_Kill(int ID);
    void com_Continue();
    void com_Pause();
    void com_Lock(char *clave, int ID);
    void com_Unlock(char *clave);
    void com_Status(char *clave);
    void com_List(char *recurso);
    void com_Deadlock();
    void com_Imprimir();

#endif /* PLANIFICADOR_H_ */

#include "../includes/init.h"

void Crear_Log();
t_config * Leer_Archivo_de_Configuracion(char * archivo);
void Configurar_Instancia(t_config * config, Instancia ** instancia);
void Enviar_Id_al_Coordinador(Instancia ** instancia);
void Recibir_Configuracion_de_Entradas(Instancia ** instancia);
void Configurar_Tabla_Entradas(Instancia ** instancia);
ALG_REEMPLAZO Algoritmo_Reemplazo_Config_to_ENUM(char * configValue);

void init(Instancia ** instancia, char ** archivos) {

    Crear_Log(archivos[0]);

    t_config * config = Leer_Archivo_de_Configuracion(archivos[1]);

    Configurar_Instancia(config, instancia);

    Enviar_Id_al_Coordinador(instancia);

    Recibir_Configuracion_de_Entradas(instancia);

    Configurar_Tabla_Entradas(instancia);
}

void Crear_Log(char * prog_name) {

    logger = log_create("instancia.log", prog_name, false, LOG_LEVEL_INFO);
    log_info(logger, "Se creo el log.");
}

t_config * Leer_Archivo_de_Configuracion(char * archivo) {

    t_config * config = config_create(archivo);

    if (config != NULL && !dictionary_is_empty(config->properties)) {

        log_info(logger, "Se cargaron las configuraciones.");
    }
    else {
        _exit_with_error(logger, -1, "No se cargaron las cofiguraciones.", NULL);
    }

    return config;
}

void Configurar_Instancia(t_config * config, Instancia ** instancia) {

    (*instancia) = crear_instancia();
    
    (*instancia)->id = config_get_int_value(config, "Instancia");
    (*instancia)->algoritmo = Algoritmo_Reemplazo_Config_to_ENUM(config_get_string_value(config, "Algoritmo"));
    char * ip_coordinador = config_get_string_value(config, "IP_Coordinador");
    char * puerto_coordinador = config_get_string_value(config, "Puerto_Coordinador");
    (*instancia)->intervalo = config_get_int_value(config, "Dump");
    (*instancia)->montaje = malloc(sizeof(strlen(config_get_string_value(config, "Montaje"))+1));
    strcpy((*instancia)->montaje, config_get_string_value(config, "Montaje"));
    (*instancia)->socket = connect_to_server(logger, ip_coordinador, puerto_coordinador);

    log_info(logger, "Configurar_Instancia: Instancia configurada.");
}

void Enviar_Id_al_Coordinador(Instancia ** instancia) {
    
    int result;
    Header * header_Coor = Crear_Header_Vacio();

    result = recv((*instancia)->socket , header_Coor , sizeof(Header) , 0);

    if(result == CLIENTE_DESCONECTADO) _exit_with_error(logger, (*instancia)->socket, "Enviar_Id_al_Coordinador: Se desconecto el Coordinador.", NULL);
    if(result == ERROR_RECV) _exit_with_error(logger, (*instancia)->socket, "Enviar_Id_al_Coordinador: Fallo la recepcion del mensaje.", NULL);
    if(header_Coor->Asunto != HANDSHAKE_COORD) _exit_with_error(logger, (*instancia)->socket, "Enviar_Id_al_Coordinador: Se esperaba como asunto HANDSHAKE_COORD.", NULL);
    
    Header * header = malloc(sizeof(Header));
    header = Crear_Header(HANDSHAKE_COORD_INST, SIN_PAYLOAD, (*instancia)->id);
    void * buf = malloc(sizeof(Header));
    memcpy(buf, header, sizeof(Header));
    write((*instancia)->socket , header , sizeof(Header));

    log_info(logger, "HANDSHAKE_COORD: Enviamos ID al Coordinador.");

    free(header_Coor);
    free(header);
    free(buf);
}

void Recibir_Configuracion_de_Entradas(Instancia ** instancia) {

    int res;
    sizeInstancia * config_Entradas = malloc(sizeof(sizeInstancia));
    Header * header = Crear_Header_Vacio();

    res = recv((*instancia)->socket, header, sizeof(Header), 0);
    if(res == CLIENTE_DESCONECTADO) _exit_with_error(logger, (*instancia)->socket, "Recibir_Configuracion_de_Entradas: Se desconectó el Coordinador.", NULL);
    if(res == ERROR_RECV) _exit_with_error(logger, (*instancia)->socket, "Recibir_Configuracion_de_Entradas: Falló la recepción del header.", NULL);
    if(header->Asunto != HANDSHAKE_COORD_INST) _exit_with_error(logger, (*instancia)->socket, "Recibir_Configuracion_de_Entradas: Se esperaba como asunto HANDSHAKE_COORD_INST.", NULL);

    res = recv((*instancia)->socket, config_Entradas, header->Length, 0);
    if(res == CLIENTE_DESCONECTADO) _exit_with_error(logger, (*instancia)->socket, "Recibir_Configuracion_de_Entradas: Se desconectó el Coordinador.", NULL);
    if(res == ERROR_RECV) _exit_with_error(logger, (*instancia)->socket, "Recibir_Configuracion_de_Entradas: Falló la recepción del contenido.", NULL);

    (*instancia)->tamanio_Entrada = config_Entradas->TamEntradas;
    (*instancia)->cantidad_Entradas = config_Entradas->CantEntradas;

    log_info(logger, "HANDSHAKE_COORD_INST: Configuracion de entradas recibida.");

    free(config_Entradas);
    free(header);
}

void Configurar_Tabla_Entradas(Instancia ** instancia) {

    (*instancia)->tamanio_Memoria = (*instancia)->cantidad_Entradas * (*instancia)->tamanio_Entrada;
    (*instancia)->memoria = malloc(sizeof(char) * (*instancia)->tamanio_Memoria);
    memset((void*)(*instancia)->memoria, 0, (*instancia)->tamanio_Memoria);

    for(int i = 0; i < (*instancia)->cantidad_Entradas; i++) {
        
        int id = i + 1;
        char * ptrEntrada = (*instancia)->memoria + (id - 1) * (*instancia)->tamanio_Entrada;
        //Crear Entrada
        t_entrada * entrada = Crear_Entrada(id, (*instancia)->tamanio_Entrada, ptrEntrada);
        //Cargarlo en la tabla

        //entrada->esVictima = id == 1;

        list_add((*instancia)->tabla_Entradas, entrada);
    }     

    log_info(logger, "HANDSHAKE_COORD_INST: Tabla de Entradas configurada.");
}

ALG_REEMPLAZO Algoritmo_Reemplazo_Config_to_ENUM(char * configValue) {

    if(strcmp(configValue, "CIRC") == 0) return CIRC;
    if(strcmp(configValue, "LRU") == 0) return LRU;
    if(strcmp(configValue, "BSU") == 0) return BSU;

    return -1;
}
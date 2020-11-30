#include "includes/esi.h"

void Destruir_Buffer(void * buffer){
    free(buffer);
}

void Enviar_Mensaje(int socket, Header * header_Payload, void* datos, t_log * logger) {
    
    Payload * payload = malloc(sizeof(Header) + header_Payload->Length);
    memcpy(payload, header_Payload, sizeof(Header));
    Destruir_Header(header_Payload);
    if (header_Payload->Length > 0) {
        memcpy(payload + sizeof(Header), &datos, header_Payload->Length);
    }
    int i = send(socket, payload, sizeof(Header) + header_Payload->Length, 0);
    printf("resultado de send: %d\n", i);
    log_info(logger, "Enviado correctamente mensaje al cliente %d", socket);
    free(payload);
    if (datos != NULL) {
        free(datos);
    }
}

void Hello_Coordinador(int socket_Coordinador, int pid, t_log * logger){
    Header * header_Coordinador = Crear_Header_Vacio();
    Header * header;
    int largo_Mensaje = 0;
    
    largo_Mensaje = recv(socket_Coordinador, header_Coordinador , sizeof(Header), 0);
    
    if(largo_Mensaje > 0 ) {
        switch(header_Coordinador->Asunto){
            case HANDSHAKE_COORD:
                header = Crear_Header(NUEVO_ESI, SIN_PAYLOAD, pid);
                Enviar_Mensaje(socket_Coordinador, header, SIN_DATOS, logger);
                break;
            default:
                break;
        }
    } else if(largo_Mensaje == 0){
        log_info(logger, "Cliente desconectado.\n");
    } else{
        log_error(logger, "Falló la recepción del mensaje.\n");
    }
}

int Handshake_Planificador(int socket_Planificador, t_log * logger){
    printf("socketPlanificador: %d\n", socket_Planificador);
    Header * header_Planificador = Crear_Header_Vacio();
    printf("tamanioHeader: %d\n",sizeof(Header));
    Header * header;
    int largo_Mensaje = 0;
    int pid = PID_SIN_ASIGNAR;

    while(pid == PID_SIN_ASIGNAR) {
        largo_Mensaje = recv(socket_Planificador, header_Planificador , sizeof(Header) , 0);
        printf("Asunto: %d, idSolicitante: %d, largoDatos: %d, identificador_Transaccion: %d\n", header_Planificador->Asunto, header_Planificador->IdSolicitante, header_Planificador->Length, header_Planificador->Identificador_Transaccion);
        printf("larog ads: %d\n", largo_Mensaje);
        if(largo_Mensaje > 0) {
            switch(header_Planificador->Asunto) {
                case HANDSHAKE_PLAN:
                    header = Crear_Header(NUEVO_ESI, SIN_PAYLOAD, SIN_ID_SOLICITANTE);
                    Enviar_Mensaje(socket_Planificador, header, SIN_DATOS, logger);
                    break;
                case ASIGNACION_PID:
                    pid = header_Planificador->Identificador_Transaccion;
                    log_info(logger,"Mi Pid es: %d\n", pid);
                    break;
                default:
                    break;
            }
        
        } else if(largo_Mensaje == 0) {
            log_info(logger, "Cliente desconectado.\n");
        } else {
            log_error(logger, "Falló la recepción del mensaje.\n");
        }
    }
    Destruir_Header(header_Planificador);
    return pid;    
}

void Recibir_Mensajes(int socket_Planificador, int socket_Coordinador, int pid, t_log * logger) {

    Header * header_Planificador = Crear_Header_Vacio();
    Header * header_Coord = Crear_Header_Vacio();
    //Sentencia * sentenciaRequest = NULL;
    //Sentencia * sentenciaResponse = NULL;

    int largo_Mensaje = 0;
    int largo_Mensaje_Coord = 0;
    Cargar_Script(logger);
    while((largo_Mensaje = recv(socket_Planificador, header_Planificador, sizeof(Header), 0) > 0 )) {

        if (!mockOn) {
            switch(header_Planificador->Asunto) {
                case EJECUTATE:
                    sleep(1);
                    log_info(logger, "El planificador me permitio ejecutar");
                    Parsear_Sentencia(socket_Planificador, pid, logger);
                    Sentencia * sentenciaRequest = Asignar_Sentencia();
                    printf("%s, %s, %d \n", sentenciaRequest->Clave, sentenciaRequest->Valor, sentenciaRequest->Operacion);
                    Enviar_Sentencia(socket_Coordinador, sentenciaRequest, EJECUTAR_SENTENCIA_REQ, pid);
                    if((largo_Mensaje_Coord = recv(socket_Coordinador, header_Coord, sizeof(Header), 0)) > 0){
                        Sentencia * sentenciaResponse = Recibir_Sentencia(socket_Coordinador);
                        printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentenciaResponse->Operacion, sentenciaResponse->Clave, sentenciaResponse->Valor, sentenciaResponse->Resultado);
                        printf("Cuasi ok \n");
                        Evaluar_Sentencia_Response(sentenciaResponse, socket_Planificador, pid, logger);
                    }
                    else if(largo_Mensaje_Coord == 0)
                    {
                        log_error(logger, "Coordinador desconectado.\n");
                    }
                    else if(largo_Mensaje_Coord == -1)
                    {
                        log_error(logger, "Falló la recepción del mensaje del coordinador.\n");
                    }                    
                    break;
                case ABORTAR_ESI:
                    printf("EL PLANI ME DIJO QUE ABORTE \n\n");
                    Terminar_Ejecucion(socket_Planificador, pid, PLAN_ABORTO, logger);
                    break;
                default:
                    break;
            }
        }
        else {
            switch(header_Planificador->Asunto) {
                case EJECUTATE:
                    sleep(1);
                    log_info(logger, "El planificador me permitio ejecutar");
                    log_debug(logger, "Ejecutando ESI con PID: %d en modo Mock");
                    MOCK_Parsear_Sentencia(socket_Planificador, pid, logger);
                    Sentencia * sentenciaRequest = Asignar_Sentencia();
                    printf("%s, %s, %d \n", sentenciaRequest->Clave, sentenciaRequest->Valor, sentenciaRequest->Operacion);
                    Sentencia * sentenciaResponse = MOCK_Recibir_Sentencia();
                    printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentenciaResponse->Operacion, sentenciaResponse->Clave, sentenciaResponse->Valor, sentenciaResponse->Resultado);
                    printf("Cuasi ok \n");
                    Evaluar_Sentencia_Response(sentenciaResponse, socket_Planificador, pid, logger);
                    //Enviar_Sentencia(socket_Planificador, sentenciaRequest, EJECUTAR_SENTENCIA_RESP, pid);
                    break;
                case ABORTAR_ESI:
                    printf("EL PLANI MOCK MOCKY ME DIJO QUE ABORTE \n\n");
                    Terminar_Ejecucion(socket_Planificador, pid, PLAN_ABORTO, logger);
                    break;
                default:
                    //Nada.
                    break;
            }
        }
    }

    if(largo_Mensaje == 0)
    {
        log_error(logger, "Planificador desconectado.\n");
    }
    else if(largo_Mensaje == -1)
    {
        log_error(logger, "Falló la recepción del mensaje.\n");
    }
    Terminar_Ejecucion(socket_Planificador, pid, PLAN_ABORTO, logger);
}

void MOCK_Parsear_Sentencia(int socket_Planificador, int pid, t_log * logger) {
    operacionActual.keyword = GET;
    operacionActual.valido = true;
    operacionActual.argumentos.GET.clave = "futbol:messi";
    if (!operacionActual.valido) {
        log_error(logger, "No se pudo parsear la sentencia MOCK: %s", sentenciasRecuperadas);
        Terminar_Ejecucion(socket_Planificador, pid, ESI_PLAN_ABORTE, logger);
        Abortar(SENTENCIA_INVALIDA);
    }
    log_info(logger ,"Se parseo la sentencia MOCK %s", Nombre_Operacion_Actual());
    switch(operacionActual.keyword) {
        case GET:
            log_info(logger ,"Con clave: %s", operacionActual.argumentos.GET.clave);
            break;
        case SET:
            log_info(logger, "Con clave: %s y valor %s", operacionActual.argumentos.SET.clave, operacionActual.argumentos.SET.valor);
            break;
        case STORE:
            log_info(logger, "Con clave: %s", operacionActual.argumentos.STORE.clave);
            break;
        default:
            break;
    }
};
                    
Sentencia * MOCK_Recibir_Sentencia() {
    Sentencia * mockSentencia = malloc(sizeof(Sentencia));
    mockSentencia->Resultado = OK;
    strcpy(mockSentencia->Clave, "");
    strcpy(mockSentencia->Valor, "");
    mockSentencia->Operacion = S_GET;
    return mockSentencia;
}


int main (int argc, char* argv[]){
    
    fileScript = NULL;
    //Se crea el logger.
    t_log * logger = log_create("esi.log", "esi", true, LOG_LEVEL_INFO);

    //Levantamos archivo de configuración.
    t_config * config = config_create("config.txt");

    if (config != NULL && !dictionary_is_empty(config->properties)) {
        log_info(logger, "Las configuraciones fueron cargadas satisfactoriamente.\n");
    }
    else {
        log_error(logger, "Las configuraciones no fueron cargadas.\n");
    }

    char * ip_Coordinador = config_get_string_value(config, "IP_Coordinador");
    char * puerto_Coordinador = config_get_string_value(config, "Puerto_Coordinador");
    char * ip_Planificador = config_get_string_value(config, "IP_Planificador");
    char * puerto_Planificador = config_get_string_value(config, "Puerto_Planificador");

    //Creamos variables a utilizar.
    int socket_Planificador = 0;
    int socket_Coordinador = 0;
    int pid;

    //Levantamos script y verificamos que sea leible.
    strcpy(stringScript ,argv[1]);
    fileScript = fopen(stringScript,"r");
    if (fileScript == NULL){
        log_error(logger, "Script inválido.\n");
        exit(ARCHIVO_INVALIDO);
    }
    //Creamos el socket para el planificador, nos conectamos y verificamos conexión exitosa.
    socket_Planificador = connect_to_server(logger, ip_Planificador, puerto_Planificador);
    printf("El socket asignado al planificador es: %d\n",socket_Planificador);
    if(socket_Planificador < 0) {
        log_error(logger, "No se pudo conectar con el Planificador.\n");
        exit(ERROR_CONEXION_PLANIFICADOR);
    }

    pid = Handshake_Planificador(socket_Planificador, logger);

    //Creamos el socket para el coordinador, nos conectamos y verificamos conexión exitosa.
    socket_Coordinador = connect_to_server(logger, ip_Coordinador, puerto_Coordinador);
    if(socket_Coordinador < 0){
        log_error(logger, "No se pudo conectar con el Coordinador.\n");
        exit(ERROR_CONEXION_COORDINADOR);
    }

    Hello_Coordinador(socket_Coordinador, pid, logger);
    
    //Esperamos instrucciones del Planificador
    Recibir_Mensajes(socket_Planificador, socket_Coordinador, pid, logger);
    
    return 0;
}

void Cargar_Script(t_log * logger) {
    char charLine = '\0';
    while(!feof(fileScript)) {
        charLine = fgetc(fileScript);
        if(charLine == '\n')
        {
            cantidadSentenciasRecuperadas++;
        }
    }
    int tamanioArchivo = ftell(fileScript);
    rewind(fileScript);

    printf("Tamanio script %d\n", tamanioArchivo);
    char * dataFile = malloc(sizeof(char) * tamanioArchivo);
    
    log_info(logger, "Se recuperaron %d sentencias", cantidadSentenciasRecuperadas);
    fread(dataFile, sizeof(char) * tamanioArchivo, 1, fileScript);
    sentenciasRecuperadas = malloc(sizeof(char) * tamanioArchivo);
    sentenciasRecuperadas = strtok(dataFile, TOKEN_FIN_SENTENCIA);
}

void Parsear_Sentencia(int socket_Planificador, int pid, t_log * logger) {
    log_info(logger, "Sentencia a parsear %s", sentenciasRecuperadas);
    operacionActual = parse(sentenciasRecuperadas);
    if (!operacionActual.valido) {
        log_error(logger, "No se pudo parsear la sentencia: %s", sentenciasRecuperadas);
        Terminar_Ejecucion(socket_Planificador, pid, ESI_PLAN_ABORTE, logger);
        Abortar(SENTENCIA_INVALIDA);
    }
    log_info(logger ,"Se parseo la sentencia %s", Nombre_Operacion_Actual());
    switch(operacionActual.keyword) {
        case GET:
            log_info(logger ,"Con clave: %s", operacionActual.argumentos.GET.clave);
            break;
        case SET:
            log_info(logger, "Con clave: %s y valor %s", operacionActual.argumentos.SET.clave, operacionActual.argumentos.SET.valor);
            break;
        case STORE:
            log_info(logger, "Con clave: %s", operacionActual.argumentos.STORE.clave);
            break;
        default:
            break;
    }
}

Sentencia * Asignar_Sentencia() {
    Sentencia * sentencia = malloc(sizeof(Sentencia));
    sentencia->Operacion = operacionActual.keyword;
    switch (operacionActual.keyword) {
        case GET:
            strcpy(sentencia->Clave, operacionActual.argumentos.GET.clave);
            strcpy(sentencia->Valor, "");
            break;
        case SET:
            strcpy(sentencia->Clave, operacionActual.argumentos.SET.clave);
            if(operacionActual.argumentos.SET.valor == NULL){
                strcpy(sentencia->Valor, "");
            }
            else{
                strcpy(sentencia->Valor, operacionActual.argumentos.SET.valor);
            }
            break;
        case STORE:
            strcpy(sentencia->Clave, operacionActual.argumentos.STORE.clave);
            strcpy(sentencia->Valor, "");
            break;
        default:
            break;
    }
    sentencia->Resultado = SIN_RESULTADO;
    return sentencia;
}

void Evaluar_Sentencia_Response(Sentencia * sentencia, int socket_Planificador, int pid, t_log * logger) {
    printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
    if(sentencia->Resultado==OK){  
        printf("NSenEj %d y SenRec en RESPONSE %d\n", numeroSentenciaEjecutar, cantidadSentenciasRecuperadas);
        if (numeroSentenciaEjecutar < cantidadSentenciasRecuperadas) {
            Enviar_Sentencia(socket_Planificador, sentencia, ESI_OK, pid);
            printf("RESULTELLIII \n");
            Avanzar_Proxima_Sentencia(socket_Planificador, pid, logger);
        }
        else {
            printf("TERMINEEEEEEE \n");
            Terminar_Ejecucion(socket_Planificador, pid, FIN_SCRIPT, logger);
        }
    }
}

/*
void Evaluar_Sentencia_Response(Sentencia * sentencia, int socket_Planificador, int pid, t_log * logger) {
    switch(sentencia->Resultado) {
        case OK:
            Enviar_Sentencia(socket_Planificador, sentencia, ESI_OK, pid);
            printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
            Avanzar_Proxima_Sentencia(socket_Planificador, pid, logger);
            break;
        case BLOQUEAR:
            break;
        case ABORTAR:
            Terminar_Ejecucion(socket_Planificador, pid, COORD_ABORTO, logger);
            break;
        default:
            break;
    }
}

void Terminar_Ejecucion(int socket_Planificador, int pid, MOTIVO_FIN motivo, t_log * logger) {
    switch(motivo){
        case FIN_SCRIPT:
            printf("TERMINE PUTITOS\n");
            Header * h = Crear_Header(ESI_PLAN_TERMINE, SIN_PAYLOAD, pid);
            Enviar_Mensaje(socket_Planificador, h, SIN_DATOS, logger);
            Destruir_Header(h);
            Abortar(motivo);
            break;
        case COORD_ABORTO:
            Abortar(motivo);
            break;
        case PLAN_ABORTO:
            Abortar(motivo);
            break;
        default:
            break;
    }

}*/

void Terminar_Ejecucion(int socket_Planificador, int pid, MOTIVO_FIN motivo, t_log * logger) {  
    if(motivo == FIN_SCRIPT) {
        printf("TERMINE PUTITOS\n");
        Header * h = Crear_Header(ESI_PLAN_TERMINE, SIN_PAYLOAD, pid);
        Enviar_Mensaje(socket_Planificador, h, SIN_DATOS, logger);
    }
    Abortar(motivo);
}

void Avanzar_Proxima_Sentencia(int socket_Planificador, int pid, t_log * logger) {

    //Parsear_Sentencia(logger);
    if (numeroSentenciaEjecutar < cantidadSentenciasRecuperadas) {
        numeroSentenciaEjecutar++;
        sentenciasRecuperadas = strtok(NULL, TOKEN_FIN_SENTENCIA);
    }
    printf("NSenEj %d y SenRec %d\n", numeroSentenciaEjecutar, cantidadSentenciasRecuperadas);

}

char * Nombre_Operacion_Actual() {
    switch(operacionActual.keyword) {
        case GET:
        return "GET";
        case SET:
        return "SET";
        case STORE:
        return "STORE";
    }
    return "Operacion no definida"; 
}

void Abortar(MOTIVO_FIN motivo) {
    printf("I'LL BE BACK (¬■_■)\n");
    exit(motivo);
}
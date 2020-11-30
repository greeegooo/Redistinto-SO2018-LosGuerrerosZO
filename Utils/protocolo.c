#include "includes/protocolo.h"

Header * Crear_Header_Vacio() {
    Header * header = malloc(sizeof(Header));
    header->Asunto = 0;
    header->IdSolicitante = 0;
    header->Length = 0;
    header->Identificador_Transaccion = 0;
    return header;
}

Header * Crear_Header(ASUNTO asunto, int length, int idSolicitante) {

	Header * header = (Header *)malloc(sizeof(Header));
	header->Asunto = asunto;
	header->Length = length;
	header->IdSolicitante = idSolicitante;
    header->Identificador_Transaccion = 0;
	return header;
}

void Destruir_Header(Header * header) {
	if (header != NULL) {
		free(header);
	}
}


char *Traducir_Operacion(int operacion){
    char* aux = NULL;
    switch(operacion){
        case S_GET:
            aux = malloc(sizeof(strlen("GET"+1)));
            strcpy(aux, "GET");
            break;
        case S_SET:
            aux = malloc(sizeof(strlen("SET"+1)));
            strcpy(aux, "SET");
            break;
        case S_STORE:
            aux = malloc(sizeof(strlen("STORE"+1)));
            strcpy(aux, "STORE");
            break;
    }
    return aux;
}
char *Traducir_Resultado(int resultado){
    char* aux = NULL;
    switch(resultado){
        case OK:
            aux = malloc(sizeof(strlen("OK"+1)));
            strcpy(aux, "OK");
            break;
        case BLOQUEAR:
            aux = malloc(sizeof(strlen("BLOQUEAR"+1)));
            strcpy(aux, "BLOQUEAR");
            break;
        case ABORTAR:
            aux = malloc(sizeof(strlen("ABORTAR"+1)));
            strcpy(aux, "ABORTAR");
            break;
    }
    return aux;
}
/*
Sentencia* Recibir_Sentencia(int cliente){
    Sentencia* sentencia = malloc(sizeof(Sentencia));
    int largo_mensaje = recv(cliente , sentencia , sizeof(Sentencia) , 0);
    printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n",sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
    if(largo_mensaje == 0)
    {
        puts("Cliente Desconectado");
    }
    else if(largo_mensaje == -1)
    {
        perror("Fallo la recepcion del mensaje");
    }
    return sentencia;
}

void Enviar_Sentencia(int cliente, Sentencia *sentencia, int opcion, int id) {

    Header *header;
    switch(opcion){
        case SENTENCIA_PLANIFICADOR_REQ:
            header = Crear_Header(SENTENCIA_PLANIFICADOR_REQ, sizeof(Sentencia), id);
            break;
        case SENTENCIA_INSTANCIA_REQ:
            header = Crear_Header(SENTENCIA_INSTANCIA_REQ, sizeof(Sentencia), id);
            break;
        case SENTENCIA_INSTANCIA_RESP:
            header = Crear_Header(SENTENCIA_INSTANCIA_RESP, sizeof(Sentencia), id);
            break;
        case EJECUTAR_SENTENCIA_RESP:
            header = Crear_Header(EJECUTAR_SENTENCIA_RESP, sizeof(Sentencia), id);
            break;
        case EJECUTAR_SENTENCIA_REQ:
            header = Crear_Header(EJECUTAR_SENTENCIA_REQ, sizeof(Sentencia), id);
            break;
        case VALOR_CLAVE:
            header = Crear_Header(VALOR_CLAVE, sizeof(Sentencia), id);
            break;
    }      

    void *buf = malloc(sizeof(Header) + header->Length);                         
    int mess_len = sizeof(Header) + header->Length;
    memcpy(buf, header, sizeof(Header));
    memcpy(buf + sizeof(Header), sentencia, sizeof(Sentencia));
    write(cliente , buf , mess_len);
    free(header);
    free(buf);
}
*/
void Enviar_Sentencia(int cliente, Sentencia *sentencia, int opcion, int id){

    Header *header;
    switch(opcion){
        case SENTENCIA_PLANIFICADOR_REQ:
            header = Crear_Header(SENTENCIA_PLANIFICADOR_REQ, sizeof(Sentencia), id);
            break;
        case SENTENCIA_INSTANCIA_REQ:
            header = Crear_Header(SENTENCIA_INSTANCIA_REQ, sizeof(Sentencia), id);
            printf("ASUNTO header: %d\n Length: %d\n IdInstancia: %d\n", header->Asunto, header->Length, header->IdSolicitante);
            break;
        case EJECUTAR_SENTENCIA_RESP:
            header = Crear_Header(EJECUTAR_SENTENCIA_RESP, sizeof(Sentencia), ID_COORDINADOR);
            break;
        case ESI_BLOQUEO:
            header = Crear_Header(ESI_BLOQUEO, sizeof(Sentencia), ID_COORDINADOR);
            break;
        case EJECUTAR_SENTENCIA_REQ:
            header = Crear_Header(EJECUTAR_SENTENCIA_REQ, sizeof(Sentencia), id);
            break;
        case CLAVES_EXISTENTES:
            header = Crear_Header(CLAVES_EXISTENTES, sizeof(Sentencia), ID_COORDINADOR);
            break;
        case CMD_STATUS:
            header = Crear_Header(CMD_STATUS, sizeof(Sentencia), id);
            break;
        case VALOR_CLAVE:
            header = Crear_Header(VALOR_CLAVE, sizeof(Sentencia), id);
            break;
        case POSIBLE_INSTANCIA:
            header = Crear_Header(POSIBLE_INSTANCIA, sizeof(Sentencia), id);
            break;
        case SENTENCIA_INSTANCIA_RESP:
            header = Crear_Header(SENTENCIA_INSTANCIA_RESP, sizeof(Sentencia), id);
            break;
        case SENTENCIA_PLANIFICADOR_RESP:
            header = Crear_Header(SENTENCIA_PLANIFICADOR_RESP, sizeof(Sentencia), id);
            break;
        case ESI_OK:
            header = Crear_Header(ESI_OK, sizeof(Sentencia), id);
            printf("ASUNTO header: %d\n Length: %d\n IdInstancia: %d\n", header->Asunto, header->Length, header->IdSolicitante);
            break;
        case ABORTAR_ESI:
            header = Crear_Header(ABORTAR_ESI, sizeof(Sentencia), id);
            break;
        default:
            break;
            
    }      
    void *buf = malloc(sizeof(Header) + header->Length);                         
    int mess_len = sizeof(Header) + header->Length;
    memcpy(buf, header, sizeof(Header));
    memcpy(buf + sizeof(Header), sentencia, sizeof(Sentencia));
    write(cliente , buf , mess_len);
    free(header);
    free(buf);
}

Sentencia* Recibir_Sentencia(int cliente){
    Sentencia* sentencia = malloc(sizeof(Sentencia));
    int largo_mensaje = recv(cliente , sentencia , sizeof(Sentencia) , 0);
    if(largo_mensaje == 0)
    {
        puts("Cliente Desconectado");
    }
    else if(largo_mensaje == -1)
    {
        perror("Fallo la recepcion del mensaje");
    }
    return sentencia;
}

Sentencia * Crear_MockSentencia(Acciones_Operacion op, char * clave, char * valor) {

    Sentencia * mockSentencia = malloc(sizeof(Sentencia));
    mockSentencia->Operacion = op;
    strcpy(mockSentencia->Clave, clave);
    strcpy(mockSentencia->Valor, valor);
    mockSentencia->Resultado = 0;
    
    printf("\n");
    printf("mockSentencia-> Operacion:%d ; Clave:%s ; Valor:%s ; Resultado:%d\n",
            mockSentencia->Operacion,
            mockSentencia->Clave,
            mockSentencia->Valor,
            mockSentencia->Resultado);
    printf("\n");

    return mockSentencia;
}
#include "includes/coordinador.h"
#include <errno.h>
#include <unistd.h>    //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

// -- Globales

ALGORITMOS_DISTRIBUCION algoritmo_distribucion;

t_list *Lista_ESI = NULL;
t_list *Lista_Instancia = NULL;
t_list *L_Planificador = NULL;
t_list *Claves_Instancias = NULL;
int Escribir_Instancia = -1;
int Id_Equitative_Load = -1;
t_config *config;
int Clave_en_Instancia_GLOBAL;
Sentencia * sentencia_global = NULL;
t_log *log_operaciones;
t_log *logger;
bool compactando = false;
int instancias_a_compactar = 0;
pthread_mutex_t sem_compactar;
pthread_mutex_t sem_ejecucion;


bool Es_Clave_Existente_En_Instancia(void * claveInstancia);
void Loguear_Sentencia(Sentencia *sentencia, int id);

AdmCliente *Crear_AdmCliente(int socket_cli, int id_cliente)
{   
    pthread_mutex_t aux;
    AdmCliente *cliente = NULL;
    pthread_mutex_init(&aux, NULL);
    cliente = malloc(sizeof(AdmCliente));
    cliente->id = id_cliente;
    cliente->Socket = socket_cli;
    cliente->Semaforo = aux;
    cliente->Valor = 0;
    return cliente;
}
/*
bool Se_Encuentra_Instancia_Activa(void * Clave_en_Instancia) {
    return ((AdmCliente *)Lista_Instancia->head->data)->id == Clave_en_Instancia);
    
}
*/

void Crear_Log_Operaciones() {

    log_operaciones = log_create("Log de Operaciones Coordinador.log", "OPERACIONES", true, LOG_LEVEL_INFO);
    log_info(log_operaciones, "Se creo el log.");
}

void Crear_Log_() {

    logger = log_create("Coordinador.log", "COORDINADOR", true, LOG_LEVEL_INFO);
    log_info(logger, "Se creo el log.");
}

Clave_Instancia *Cargar_Clave(char* clave, int instancia){
    Clave_Instancia *aux = NULL;
    aux = malloc(sizeof(Clave_Instancia));
    strcpy(aux->Clave, clave);
    aux->IdInstancia = instancia;
    aux->EntradasQueOcupa = 0;
    return aux;
}

void Crear_Diccionario()
{
    diccionarioRecursosPedidos = dictionary_create();
}

void Eliminar_Diccionario()
{
    dictionary_destroy(diccionarioRecursosPedidos);
}

bool Recurso_Bloqueado(char *clave)
{
    return dictionary_has_key(diccionarioRecursosPedidos, clave);
}

int Id_Reservo_Recurso(char *clave)
{
    return (int)dictionary_get(diccionarioRecursosPedidos, clave);
}

void enviar_dimensiones_instancia(int cliente)
{

    Header *header;
    sizeInstancia *dimensiones = malloc(sizeof(sizeInstancia));
    dimensiones->CantEntradas = config_get_int_value(config, "Cantidad_Entradas");
    dimensiones->TamEntradas = config_get_int_value(config, "Tamanio_Entradas");
    header = Crear_Header(HANDSHAKE_COORD_INST, sizeof(sizeInstancia), -1);
    void *buf = malloc(sizeof(Header) + header->Length);
    int mess_len = sizeof(Header) + header->Length;
    memcpy(buf, header, sizeof(Header));
    memcpy(buf + sizeof(Header), dimensiones, sizeof(sizeInstancia));
    write(cliente, buf, mess_len);
    log_info(logger, "Se enviaron las dimensiones a la instancia satisfactoriamente.\n");
    //printf("Cantidad entradas: %d\nTamanio entradas: %d\n", dimensiones->CantEntradas, dimensiones->TamEntradas);
    //printf("ASUNTO header: %d\n Length: %d\n IdInstancia: %d\n", header->Asunto, header->Length, header->IdSolicitante);
    free(header);
    free(dimensiones);
    free(buf);
}

int Socket_Por_Id(int id, int opcion)
{
    int socket = -1;
    t_link_element *aux = NULL;
    int largo;
    switch (opcion)
    {
        case INSTANCIA:
            aux = Lista_Instancia->head;
            largo = Lista_Instancia->elements_count;
            break;
        case ESI:
            aux = Lista_ESI->head;
            largo = Lista_ESI->elements_count;
            break;
        case PLANIFICADOR:
            aux = L_Planificador->head;
            largo = L_Planificador->elements_count;
            break;
    }

    for (int i = 0; i < largo; i++)
    {
        if (((AdmCliente *)aux->data)->id == id)
        {
            socket = ((AdmCliente *)aux->data)->Socket;
            return socket;
        }
        else
            aux = aux->next;
    }
    
    return socket;
}

//NO ESTA PROBADO

pthread_mutex_t Semaforo_Por_Id(int id, int opcion)
{
    pthread_mutex_t semaforo;
    t_link_element *aux = NULL;
    int largo;
    switch (opcion)
    {
        case INSTANCIA:
            aux = Lista_Instancia->head;
            largo = Lista_Instancia->elements_count;
            break;
        case ESI:
            aux = Lista_ESI->head;
            largo = Lista_ESI->elements_count;
            break;
        case PLANIFICADOR:
            aux = L_Planificador->head;
            largo = L_Planificador->elements_count;
            break;
    }

    for (int i = 0; i < largo; i++)
    {
        if (((AdmCliente *)aux->data)->id == id)
        {
            semaforo = ((AdmCliente *)aux->data)->Semaforo;
            return semaforo;
        }
        else
            aux = aux->next;
    }
    
    return semaforo;
}

//NO ESTA PROBADO
void Bloquear_Semaforo_Instancia(int id){
    t_link_element *aux = NULL;
    int largo;
    aux = Lista_Instancia->head;
    largo = Lista_Instancia->elements_count;
    for (int i = 0; i < largo; i++)
        {
            if (((AdmCliente *)aux->data)->id != id)
            {
                pthread_mutex_lock(&(((AdmCliente *)aux->data)->Semaforo));
                aux = aux->next;                    
            }
        }
}

//NO ESTA PROBADO
void Bloquear_Semaforo_ESI(int id){
    t_link_element *aux = NULL;
    int largo;
    aux = Lista_ESI->head;
    largo = Lista_ESI->elements_count;
    for (int i = 0; i < largo; i++)
        {
            if (((AdmCliente *)aux->data)->id != id)
            {
                pthread_mutex_lock(&(((AdmCliente *)aux->data)->Semaforo));
                aux = aux->next;                    
            }
        }
}

//NO ESTA PROBADO
void Bloquear_Semaforo_Planificador(int id){
    t_link_element *aux = NULL;
    aux = L_Planificador->head;
    if (((AdmCliente *)aux->data)->id != id)
        pthread_mutex_lock(&(((AdmCliente *)aux->data)->Semaforo));
        
}

//NO ESTA PROBADO
void Todos_Menos_Id(int id){
    Bloquear_Semaforo_Instancia(id);
    Bloquear_Semaforo_ESI(id);
    Bloquear_Semaforo_Planificador(id);
}

//NO ESTA PROBADO
void Todos_Menos_Instancias(int opcion){
    t_link_element *aux = NULL;
    int largo;
    aux = L_Planificador->head;
    largo = L_Planificador->elements_count;
    switch(opcion){
        case LOCK:
            pthread_mutex_lock(&(((AdmCliente *)aux->data)->Semaforo));
            break;
        case UNLOCK:
            pthread_mutex_unlock(&(((AdmCliente *)aux->data)->Semaforo));
            break;
        default:
            break;
    }
    aux = Lista_ESI->head;
    largo = Lista_ESI->elements_count;
    for (int i = 0; i < largo; i++)
        {
            switch(opcion){
                case LOCK:
                    pthread_mutex_lock(&(((AdmCliente *)aux->data)->Semaforo));
                    break;
                case UNLOCK:
                    pthread_mutex_unlock(&(((AdmCliente *)aux->data)->Semaforo));
                    break;
                default:
                    break; 
            }
            aux = aux->next;                
        }
}

//NO ESTA PROBADO
void Semaforos(int id, int opcion, int opcion2){
    switch(opcion){
        case TODOS_MENOS_ID:
            Todos_Menos_Id(id);
            break;
        case TODOS_MENOS_INSTANCIAS:
            Todos_Menos_Instancias(opcion2);
            break;
        default:
            break;
    }
}

int Id_Por_Socket(int socket, int opcion)
{
    int id = -1;
    t_link_element *aux = NULL;
    int largo;
    switch (opcion)
    {
    case INSTANCIA:
        aux = Lista_Instancia->head;
        largo = Lista_Instancia->elements_count;
        break;
    case ESI:
        aux = Lista_ESI->head;
        largo = Lista_ESI->elements_count;
        break;
    case PLANIFICADOR:
        aux = L_Planificador->head;
        largo = L_Planificador->elements_count;
        break;
    }
    for (int i = 0; i < largo; i++)
    {
        if (((AdmCliente *)aux->data)->Socket == socket)
        {
            id = ((AdmCliente *)aux->data)->id;
            return id;
        }
        else
        {
            aux = aux->next;
        }
    }
    return id;
}



int Id_Por_Clave(char* clave)
{
    int id = -1;
    t_link_element *aux = NULL;
    int largo;
        aux = Claves_Instancias->head;
        largo = Claves_Instancias->elements_count;
    for (int i = 0; i < largo; i++)
    {
        if (strcmp(((Clave_Instancia *)aux->data)->Clave, clave) == 0)
        {
            id = ((Clave_Instancia *)aux->data)->IdInstancia;
            return id;
        }
        else
        {
            aux = aux->next;
        }
    }
    return id;
}

int Index_Por_Clave(char *clave)
{
    int index = -1;
    t_link_element *aux = NULL;
    int largo;
    aux = Claves_Instancias->head;
    largo = Claves_Instancias->elements_count;
    for (int i = 0; i < largo; i++)
    {
        if (strcmp(((Clave_Instancia *)aux->data)->Clave, clave) == 0)
        {
            index = i;
            return index;
        }
        else
        {
            aux = aux->next;
        }
    }
    //free(aux);
    return index;
}

int Index_Por_Socket(int socket, int opcion)
{
    int index = -1;
    t_link_element *aux = NULL;
    int largo;
    switch (opcion)
    {
    case INSTANCIA:
        aux = Lista_Instancia->head;
        largo = Lista_Instancia->elements_count;
        break;
    case ESI:
        aux = Lista_ESI->head;
        largo = Lista_ESI->elements_count;
        break;
    case PLANIFICADOR:
        aux = L_Planificador->head;
        largo = L_Planificador->elements_count;
    break;
    }
    for (int i = 0; i < largo; i++)
    {
        if (((AdmCliente *)aux->data)->Socket == socket)
        {
            index = i;
            return index;
        }
        else
        {
            aux = aux->next;
        }
    }
    //free(aux);
    return index;
}

int Index_Por_Id(int id, int opcion)
{
    int index = -1;
    t_link_element *aux = NULL;
    int largo;
    switch (opcion)
    {
    case INSTANCIA:
        aux = Lista_Instancia->head;
        largo = Lista_Instancia->elements_count;
        break;
    case ESI:
        aux = Lista_ESI->head;
        largo = Lista_ESI->elements_count;
        break;
    case PLANIFICADOR:
        aux = L_Planificador->head;
        largo = L_Planificador->elements_count;
    break;
    }
    for (int i = 0; i < largo; i++)
    {
        if (((AdmCliente *)aux->data)->id == id)
        {
            index = i;
            return index;
        }
        else
        {
            aux = aux->next;
        }
    }
    //free(aux);
    return index;
}

void Resultado_Evaluacion_Planificador(Sentencia *sentencia, int id)
{
    int cliente = 0;
    switch (sentencia->Resultado)
    {
        case OK:
            cliente = Socket_Por_Id(Escribir_Instancia, INSTANCIA);
            //printf("El socket de la instancia:%d es:%d\n", Escribir_Instancia, cliente);
            Enviar_Sentencia(cliente, sentencia, SENTENCIA_INSTANCIA_REQ, id);
            break;
        case BLOQUEAR:
            cliente = Socket_Por_Id(id, ESI);
            Enviar_Sentencia(cliente, sentencia, EJECUTAR_SENTENCIA_RESP, ID_COORDINADOR);
            break;
        case ABORTAR:
            cliente = Socket_Por_Id(id, ESI);
            Enviar_Sentencia(cliente, sentencia, EJECUTAR_SENTENCIA_RESP, ID_COORDINADOR);
            break;
        default:
            break;
        
    }
}

void Handshake_Coordinador(int cliente)
{

    Header *headerCoor;
    headerCoor = Crear_Header(HANDSHAKE_COORD, SIN_PAYLOAD, ID_COORDINADOR);
    write(cliente, headerCoor, sizeof(Header));
    log_info(logger, "Se envio correctamente el asunto HANDSHAKE_COORD");
    free(headerCoor);
}

void Imprimir_Lista(int opcion)
{
    t_link_element *aux = NULL;
    int largo;
    switch (opcion)
    {
    case INSTANCIA:
        aux = Lista_Instancia->head;
        largo = Lista_Instancia->elements_count;
        break;
    case ESI:
        aux = Lista_ESI->head;
        largo = Lista_ESI->elements_count;
        break;
    case PLANIFICADOR:
        aux = L_Planificador->head;
        largo = L_Planificador->elements_count;
        break;
        
        }
    for (int i = 0; i < largo; i++){
            printf("ID: %d\nSocket: %d\nValor: %d\n", ((AdmCliente *)aux->data)->id, ((AdmCliente *)aux->data)->Socket, ((AdmCliente *)aux->data)->Valor);
            if (aux->next != NULL)
                aux = aux->next;
    //free(aux);
    }
}

void Imprimir_Lista_Clave()
{
    //printf("ESTOY EN LA FUNCION IMPRIMIR LISTA CLAVES\n");
    t_link_element *aux = NULL;
    int largo;
        aux = Claves_Instancias->head;
        largo = Claves_Instancias->elements_count;
    for (int i = 0; i < largo; i++){
            printf("Clave: %s\nId instancia%d\nEntradas Que Ocupa:%d\n", ((Clave_Instancia *)aux->data)->Clave, ((Clave_Instancia *)aux->data)->IdInstancia,((Clave_Instancia *)aux->data)->EntradasQueOcupa);
            if (aux->next != NULL)
                aux = aux->next;
    //free(aux);
    }
}

void Cargar_Estructura_Administrativa(int cliente, int IdSolicitante, int opcion)
{

    AdmCliente *ClienteAdm;
    ClienteAdm = Crear_AdmCliente(cliente, IdSolicitante);
    switch (opcion)
    {
    case PLANIFICADOR:
        list_add(L_Planificador, ClienteAdm);
        log_info(logger, "Se cargo correctamente el Planificador en la estructura administrativa.\n");
        break;
    case INSTANCIA:
        list_add(Lista_Instancia, ClienteAdm);
        log_info(logger, "Se cargo correctamente la Instancia %d en la estructura administrativa.\n", IdSolicitante);
        break;
    case ESI:
        list_add(Lista_ESI, ClienteAdm);
        log_info(logger, "Se cargo correctamente el ESI %d en la estructura administrativa.\n", IdSolicitante);
        break;
    }
    //free(ClienteAdm);
}

void Eliminar_Elemento_Lista(int index, int opcion)
{

    t_list *aux = NULL;
    t_list *aux2 = NULL;
    switch (opcion)
    {
    case INSTANCIA:
        aux = list_duplicate(Lista_Instancia);
        break;
    case ESI:
        aux = list_duplicate(Lista_ESI);
        break;
    case PLANIFICADOR:
        aux = list_duplicate(L_Planificador);
        break;
    case CLAVES:
        aux = list_duplicate(Claves_Instancias);
        break;    
    }
    aux2 = aux;
    list_remove(aux, index);
    switch (opcion)
    {
    case INSTANCIA:
        Lista_Instancia = aux2;
        break;
    case ESI:
        Lista_ESI = aux2;
        break;
    case PLANIFICADOR:
        L_Planificador = aux2;
        break;
    case CLAVES:
        Claves_Instancias = aux2;
        break;
    }
}

void Algoritmo_Distribucion(char *algoritmo)
{
    if (strcmp(algoritmo, "KE") == 0)
    {
        algoritmo_distribucion = KE;
    }
    else if (strcmp(algoritmo, "LSU") == 0)
    {
        algoritmo_distribucion = LSU;
    }
    else
    {
        algoritmo_distribucion = EL;
    }

    free(algoritmo);
}

void Distribuir_Letras(){
    int cant_letras;
    int cantidad_instancias = Lista_Instancia->elements_count;
    int valor_anterior;
    int i;
    t_link_element *aux = Lista_Instancia->head;
    cant_letras = LETRAS_TOTALES / cantidad_instancias + 1;
    for(i = 0; i < cantidad_instancias; i++){
        if(i == 0){   
            ((AdmCliente *)aux->data)->Valor = LETRA_A + cant_letras;
            //log_info(logger, "Instancia N %d - Letras %s - %s.\n", ((AdmCliente *)aux->data)->id, LETRA_A, ((AdmCliente *)aux->data)->Valor);
            valor_anterior = ((AdmCliente *)aux->data)->Valor;
            aux = aux->next;
        }else{
            ((AdmCliente *)aux->data)->Valor = valor_anterior + cant_letras;
            valor_anterior = ((AdmCliente *)aux->data)->Valor;
            aux = aux->next;
        }
    }
    log_info(logger, "Se redistribuyeron correctamente las instancias.\n");
}


void Key_Explicit(Sentencia *sentencia){
    int letra_clave = tolower(sentencia->Clave[0]);
    int i, valor_anterior;
    t_link_element *aux = Lista_Instancia->head;
    int cantidad_instancias = Lista_Instancia->elements_count;
    for(i = 0; i < cantidad_instancias; i++){
       if(i == 0){
           if(letra_clave >= LETRA_A && letra_clave <= ((AdmCliente *)aux->data)->Valor){
               Escribir_Instancia = ((AdmCliente *)aux->data)->id;
               i = cantidad_instancias;
           }else{
               valor_anterior = ((AdmCliente *)aux->data)->Valor;
               aux = aux->next;
           }   
        }else{
            if(letra_clave >= valor_anterior && letra_clave <= ((AdmCliente *)aux->data)->Valor){
               Escribir_Instancia = ((AdmCliente *)aux->data)->id;
               i = cantidad_instancias; 
            }else{
               valor_anterior = ((AdmCliente *)aux->data)->Valor;
               aux = aux->next;
           } 
            
        } 
    }
}

void Equitative_Load(int id, int opcion, int opcion2){
    t_list *aux = NULL;
    t_list *aux2 = NULL;
    AdmCliente* aux3 = NULL;
    int index;
    switch(opcion){
        case EQUITATIVE:
            Escribir_Instancia = ((AdmCliente *)Lista_Instancia->head->data)->id;
            if(opcion2 == CON_EFECTO){
                aux = list_duplicate(Lista_Instancia);
                aux3 = list_remove(aux, 0);
                list_add(aux, aux3);
                aux2 = aux;
                Lista_Instancia = aux2;
                //Imprimir_Lista(INSTANCIA);
            }
            break;
        case DESEMPATE:
            Escribir_Instancia = id;
            if(opcion2 == CON_EFECTO){
                aux = list_duplicate(Lista_Instancia);
                index = Index_Por_Id(id, INSTANCIA);
                aux3 = list_remove(aux, index);
                list_add(aux, aux3);
                aux2 = aux;
                Lista_Instancia = aux2;
                //Imprimir_Lista(INSTANCIA);
            }
            break;
    }

   // list_destroy(aux);
   //ree(aux);
   //free(aux2);
    //list_destroy(aux2);

}
void Least_Space_Used(int opcion){
    t_list *lsu = NULL;
    t_link_element *aux = Lista_Instancia->head;
    int cantidad_instancias = Lista_Instancia->elements_count;
    lsu = list_create();
    list_add(lsu,((AdmCliente *)aux->data));
    int valor = ((AdmCliente *)aux->data)->Valor;
    aux = aux->next;
    for(int i = 0; i < cantidad_instancias -1; i++){
        if(((AdmCliente *)aux->data)->Valor == valor){
            list_add(lsu, ((AdmCliente *)aux->data));
            aux = aux->next;
        }else{
            if(((AdmCliente *)aux->data)->Valor < valor){
                valor = ((AdmCliente *)aux->data)->Valor;
                list_clean(lsu);
                list_add(lsu,((AdmCliente *)aux->data));
                aux = aux->next;
            }else{
                aux = aux->next;
            }
        }
    }
    if(lsu->elements_count > 1){
        if(opcion == CON_EFECTO){
            Equitative_Load(((AdmCliente *)lsu->head->data)->id, DESEMPATE, CON_EFECTO);
            log_info(logger, "Se desempato con Equitative Load - La instancia asignada fue:%d", Escribir_Instancia);
        }else{
            Equitative_Load(((AdmCliente *)lsu->head->data)->id, DESEMPATE, SIMULACION);
            //printf("pase despues\n");
        }
    }else{
            Escribir_Instancia = ((AdmCliente *)lsu->head->data)->id;
            log_info(logger, "Por algoritmo Least Space Used la instancia asignada fue:%d", Escribir_Instancia);
    }
    free(lsu);
}



void Algoritmos_Distribucion(Sentencia *sentencia, int opcion, int opcion2)
{
    switch (opcion){
    case LSU:
        Least_Space_Used(opcion2);
        break;
    case EL:
        Equitative_Load(0, EQUITATIVE, opcion2);
        break;
    case KE:
        Key_Explicit(sentencia);
        break;
    case REDISTRIBUCION:
        Distribuir_Letras();
        break;
    }
}
int Existencia_Instancia(int id){

    int i, j, cantidad_instancias, cantidad_claves;
    t_link_element *aux = Lista_Instancia->head;
    t_link_element *aux2 = Claves_Instancias->head;
    cantidad_instancias = Lista_Instancia->elements_count;
    cantidad_claves = Claves_Instancias->elements_count;
    for(i = 0; i < cantidad_instancias; i++){
        if(id == ((AdmCliente *)aux->data)->id){
            return INSTANCIA_EXISTENTE; 
        }
        else{
            aux = aux->next; 
        }
    }
    for(j = 0; j < cantidad_claves; j++){
        if(id == ((Clave_Instancia *)aux2->data)->IdInstancia){
            return INSTANCIA_EXISTIA;
        }else{
                aux2 = aux2->next;
            }
    }

    return INSTANCIA_NO_EXISTE;
}

int Instancia_Disponible(int id){
    int i, cantidad_instancias;
    t_link_element *aux = Lista_Instancia->head;
    cantidad_instancias = Lista_Instancia->elements_count;
    for(i = 0; i < cantidad_instancias; i++){
        if(id == ((AdmCliente *)aux->data)->id){
            return INSTANCIA_EXISTENTE; 
        }else{
            aux = aux->next;
        }
    }
    return INSTANCIA_NO_EXISTE;
}


int Existencia_Clave(Sentencia *sentencia){
    int i, cantidad_claves;
    t_link_element *aux = Claves_Instancias->head;
    cantidad_claves = Claves_Instancias->elements_count;
    for(i = 0; i < cantidad_claves; i++){
        if(strcmp(sentencia->Clave, ((Clave_Instancia *)aux->data)->Clave) == 0){
            return ((Clave_Instancia *)aux->data)->IdInstancia; 
        }else{
            aux = aux->next;
        }
    }
    return CLAVE_INEXISTENTE;
}
//NO ESTA PROBADO
Sentencia *Cargar_Sentencia_Instancia(Clave_Instancia* clave_instancia){
    Sentencia* sentencia = NULL;
    sentencia = malloc(sizeof(Sentencia));
    sentencia->Operacion = -1;
    strcpy(sentencia->Clave, clave_instancia->Clave);
    sentencia->Resultado = -1;
    return sentencia;
}
//NO ESTA PROBADO
void enviar_datos_instancia(int cliente){
    int i, cantidad_claves, id, index;
    Sentencia *sentencia = NULL;
    t_link_element *aux = Claves_Instancias->head;
    cantidad_claves = Claves_Instancias->elements_count;
    t_list *Claves_de_una_Instancia = NULL;
    Claves_de_una_Instancia = list_create();
    id = Id_Por_Socket(cliente, INSTANCIA);
    Header *header = malloc(sizeof(Header));
    for(i = 0; i < cantidad_claves; i++){
        if(((Clave_Instancia *)aux->data)->IdInstancia == id){
            list_add(Claves_de_una_Instancia, ((Clave_Instancia *)aux->data));
            aux = aux->next;
        }else
            aux = aux->next;
    }
    aux = Claves_de_una_Instancia->head;
    for(i = 0; i < Claves_de_una_Instancia->elements_count; i++){
        sentencia = Cargar_Sentencia_Instancia(((Clave_Instancia *)aux->data));
        Enviar_Sentencia(cliente, sentencia, CLAVES_EXISTENTES, ID_COORDINADOR);
        recv(cliente, header, sizeof(Header), 0);
        log_info(logger, "ASUNTO header: - \n Length: - \n IdInstancia: %d\n", header->Asunto, header->Length, header->IdSolicitante);
        if(header->Asunto == CLAVES_EXISTENTES_ABORTAR){
            index = Index_Por_Clave(sentencia->Clave);
            Eliminar_Elemento_Lista(index, CLAVES);
            if(Claves_Instancias->elements_count > 0){
                //Imprimir_Lista_Clave();
            }
        }
        memset(header, 0, sizeof(Header));
        memset(sentencia, 0, sizeof(Sentencia));
        aux = aux->next;
    }
    free(header);
    free(sentencia);
    free(Claves_de_una_Instancia);
    //list_destroy(Claves_de_una_Instancia);
}


void Resultado_Evaluacion_Coordinador(Sentencia* sentencia, int id_ESI, int Clave_en_Instancia){
    int cliente = 0;
    Sentencia *aux;

    //PRUEBAS
    //int socketPrueba; 
    //FIN PRUEBAS
    switch (sentencia->Operacion){
        case S_GET:
                log_info(logger, "Operacion GET\n");
            if(Lista_Instancia->elements_count > SIN_ELEMENTOS){
                if(Clave_en_Instancia != CLAVE_INEXISTENTE){
                    if(Instancia_Disponible(Clave_en_Instancia) == INSTANCIA_NO_EXISTE){
                        log_info(logger, "Clave existente e instancia NO disponible\n");
                        int index = Index_Por_Clave(sentencia->Clave);
                        Eliminar_Elemento_Lista(index, CLAVES);
                        Algoritmos_Distribucion(sentencia, algoritmo_distribucion, CON_EFECTO);
                        log_info(logger, "La instancia a la que debe ir es:%d\n", Escribir_Instancia);
                        log_info(logger, "EL GET SE PROCESO CORRECTAMENTE EN EL COORDINADOR, SE ENVIO AL PLANIFICADOR\n");
                        Enviar_Sentencia(((AdmCliente *)L_Planificador->head->data)->Socket, sentencia, SENTENCIA_PLANIFICADOR_REQ, id_ESI);
                    }else{
                        log_info(logger, "Clave existente e instancia disponible\n");
                        Escribir_Instancia = Clave_en_Instancia;
                        log_info(logger, "La instancia a la que debe ir es:%d\n", Escribir_Instancia);
                        log_info(logger, "EL GET SE PROCESO CORRECTAMENTE EN EL COORDINADOR, SE ENVIO AL PLANIFICADOR\n");
                        Enviar_Sentencia(((AdmCliente *)L_Planificador->head->data)->Socket, sentencia, SENTENCIA_PLANIFICADOR_REQ, id_ESI);
                    }
                }else{
                    log_info(logger,"Clave INEXISTENTE\n");
                    //evaluar en que instancia deberia ser guardado - guardar instancia id en variable global Escribir_Instancia
                    Algoritmos_Distribucion(sentencia, algoritmo_distribucion, CON_EFECTO);
                    log_info(logger, "La instancia a la que debe ir es:%d\n", Escribir_Instancia);
                    log_info(logger, "EL GET SE PROCESO CORRECTAMENTE EN EL COORDINADOR, SE ENVIO AL PLANIFICADOR\n");
                    //Distribucion_Instancias(Distribucion_Instancias);

                    //printf("Operacion:%s\nClave:%s\nValor:%s\nResultado:%d\n",sentencia_ESI->Operacion, sentencia_ESI->Clave, sentencia_ESI->Valor, sentencia_ESI->Resultado);
                    //Se envia al planificador la sentencia


                    Enviar_Sentencia(((AdmCliente *)L_Planificador->head->data)->Socket, sentencia, SENTENCIA_PLANIFICADOR_REQ, id_ESI);
                    
                    
                    /* Se pregunta al Planificador si se puede ejecutar sentencia asunto: SENTENCIA_PLANIFICADOR_REQ
                        enviar el struct Sentencia
                        */
                    }
            }else{
                log_error(logger, "Aborto por Instancias Desconectadas\n");
                sentencia->Resultado = ABORTAR;
                aux = sentencia;
                //printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
                cliente = Socket_Por_Id(id_ESI, ESI);
                //aca cambie el workflow, los abortos los mando al planificador y el deberia informarle al esi paralelamente le envio al esi una respuesta para liberar el recv
                Enviar_Sentencia(cliente, sentencia, EJECUTAR_SENTENCIA_RESP, ID_COORDINADOR);
                Enviar_Sentencia(((AdmCliente *)L_Planificador->head->data)->Socket, sentencia, ABORTAR_ESI, id_ESI);
                Loguear_Sentencia(sentencia, id_ESI);
            }
            break;
        default:
            log_info(logger, "Se ejecuto SET O STORE\n");
            if(Clave_en_Instancia != CLAVE_INEXISTENTE){
                if(Instancia_Disponible(Clave_en_Instancia) == INSTANCIA_EXISTENTE){
                    Escribir_Instancia = Clave_en_Instancia;
                    log_info(logger, "La instancia a la que debe ir es:%d\n", Escribir_Instancia);
                    log_info(logger, "EL SET O STORE SE PROCESO CORRECTAMENTE EN EL COORDINADOR, SE ENVIO AL PLANIFICADOR\n");
                    Enviar_Sentencia(((AdmCliente *)L_Planificador->head->data)->Socket, sentencia, SENTENCIA_PLANIFICADOR_REQ, id_ESI);
                    /*PRUEBAS SIN PLANIFICADOR
                    socketPrueba = Socket_Por_Id(Escribir_Instancia, INSTANCIA);
                    printf("Socket: %d\n", socketPrueba);
                    printf("Sentencia\nOperacion: %d\nClave: %s\nValor: %s\nResultado: %d\n",sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
                    Enviar_Sentencia(socketPrueba, sentencia, EJECUTAR_SENTENCIA_REQ, id_ESI);
                    */
                    //free(sentencia);
                }else{
                    log_error(logger, "Aborto por clave inaccesible\n");
                    sentencia->Resultado = ABORTAR;
                    aux = sentencia;
                    cliente = Socket_Por_Id(id_ESI, ESI);
                    //eliminar de claves_instancias la clave
                    int index = Index_Por_Clave(sentencia->Clave);
                    Eliminar_Elemento_Lista(index, CLAVES);
                    //aca cambie el workflow, los abortos los mando al planificador y el deberia informarle al esi paralelamente le envio al esi una respuesta para liberar el recv
                    Enviar_Sentencia(cliente, aux, EJECUTAR_SENTENCIA_RESP, ID_COORDINADOR);
                    Enviar_Sentencia(((AdmCliente *)L_Planificador->head->data)->Socket, sentencia, ABORTAR_ESI, id_ESI);
                    Loguear_Sentencia(sentencia, id_ESI);

                    
                }
            }else{
                log_error(logger, "Aborto por clave no identificada\n");
                sentencia->Resultado = ABORTAR;
                aux = sentencia;
                //printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
                cliente = Socket_Por_Id(id_ESI, ESI);
                //aca cambie el workflow, los abortos los mando al planificador y el deberia informarle al esi paralelamente le envio al esi una respuesta para liberar el recv
                Enviar_Sentencia(cliente, sentencia, EJECUTAR_SENTENCIA_RESP, ID_COORDINADOR);
                Enviar_Sentencia(((AdmCliente *)L_Planificador->head->data)->Socket, sentencia, ABORTAR_ESI, id_ESI);
                Loguear_Sentencia(sentencia, id_ESI);
                
            }
            break;
            
    }
    //Imprimir_Lista_Clave();
}
                    
void Loguear_Sentencia(Sentencia *sentencia, int id){

    char* operacion = NULL;
    char* resultado = NULL;

    operacion = Traducir_Operacion(sentencia->Operacion);
    resultado = Traducir_Resultado(sentencia->Resultado);
    
    switch(sentencia->Operacion){
        case S_GET:
        case S_STORE:
            if(sentencia->Resultado == ABORTAR){
                log_error(log_operaciones, "ESI %d - Sentencia %s %s  %s\n", id, operacion, sentencia->Clave, resultado);
            }else{
                log_info(log_operaciones, "ESI %d - Sentencia %s %s  %s\n", id, operacion, sentencia->Clave, resultado);
            }
            break;
        case S_SET:
            if(sentencia->Resultado == ABORTAR){
                log_error(log_operaciones, "ESI %d - Sentencia %s %s %s  %s\n", id, operacion, sentencia->Clave, sentencia->Valor, resultado);
            }else{
                log_info(log_operaciones, "ESI %d - Sentencia %s %s %s  %s\n", id, operacion, sentencia->Clave, sentencia->Valor, resultado);
            }
            break;
    }
    free(operacion);
    free(resultado);
}

int Entradas_necesarias(char * valor, int tamanio) {
    return (int)ceil((float)strlen(valor) / tamanio);
}

void Validar_Compactacion(){
    if(compactando){
        instancias_a_compactar--;
        if(instancias_a_compactar == 0)
            compactando = false;
    }else
        instancias_a_compactar = Lista_Instancia->elements_count;
}

void Redistribuir(int id_Socket, int algoritmo, int index){
    if((Lista_Instancia->elements_count-1) > SIN_ELEMENTOS){
        Eliminar_Elemento_Lista(index, INSTANCIA);
        Algoritmos_Distribucion(NULL, REDISTRIBUCION, CON_EFECTO);                          
        //Imprimir_Lista(INSTANCIA);
        Validar_Compactacion();           
    }else{
        Eliminar_Elemento_Lista(index, INSTANCIA);
        Validar_Compactacion();
    }
}

int Sumatoria_Entradas_Que_Ocupa(int id){
    int i, sumatoria = 0;
    t_link_element *aux2 = Claves_Instancias->head;
    int cantidad_claves = Claves_Instancias->elements_count;
    for(i = 0; i < cantidad_claves; i++){
        if (((Clave_Instancia *)aux2->data)->IdInstancia == id){
            sumatoria += ((Clave_Instancia *)aux2->data)->EntradasQueOcupa;
            aux2 = aux2->next;        
        }else{
            aux2 = aux2->next;
        }
    }
    return sumatoria;
}

void Actualizar_Clave_Instancia_LSU(Sentencia *sentencia){
    int tamanio = config_get_int_value(config, "Tamanio_Entradas");
    int id = Id_Por_Clave(sentencia->Clave);
    int i;
    t_link_element *aux = Lista_Instancia->head;
    int cantidad_instancias = Lista_Instancia->elements_count;
    t_link_element *aux2 = Claves_Instancias->head;
    int cantidad_claves = Claves_Instancias->elements_count;
    for(i = 0; i < cantidad_claves; i++){
        if (strcmp(((Clave_Instancia *)aux2->data)->Clave, sentencia->Clave) == 0){
            ((Clave_Instancia *)aux2->data)->EntradasQueOcupa = Entradas_necesarias(sentencia->Valor, tamanio);
            log_info(log_operaciones, "Cuanto ocupe por el SET que hice:%d\n", Entradas_necesarias(sentencia->Valor, tamanio));
            i = cantidad_claves;
        }else{
            aux2 = aux2->next;
        }
    }
    for(i = 0; i < cantidad_instancias; i++){
        if(((AdmCliente *)aux->data)->id == id){
           ((AdmCliente *)aux->data)->Valor = Sumatoria_Entradas_Que_Ocupa(id);
           log_info(log_operaciones, "Cuanto lleva ocupado en la instnacia:%d\n", ((AdmCliente *)aux->data)->Valor);
           i = cantidad_instancias;
        }else
            aux = aux->next;
    }
}
void Compactar_Instancias_Menos_Socket(int cliente){
    int i;
    t_link_element *aux = Lista_Instancia->head;
    int cantidad_instancias = Lista_Instancia->elements_count;
    Header *header;
    header = Crear_Header(COMPACTAR, SIN_PAYLOAD, ID_COORDINADOR);
    void *buf = malloc(sizeof(Header) + header->Length);                         
    int mess_len = sizeof(Header) + header->Length;
    memcpy(buf, header, sizeof(Header));
    for(i = 0; i < cantidad_instancias; i++){
        if(((AdmCliente *)aux->data)->Socket != cliente){
            write(((AdmCliente *)aux->data)->Socket , buf , mess_len);
            //sleep(1);
            aux = aux->next;
        }else
            aux = aux->next;   
    }   
}
void Resultado_Evaluacion_Instancia(Sentencia *sentencia, int id, int socket){
    if(sentencia->Resultado == ABORTAR){
        Enviar_Sentencia(socket, sentencia, EJECUTAR_SENTENCIA_RESP, ID_COORDINADOR);
        sleep(1);
        Enviar_Sentencia(((AdmCliente *)L_Planificador->head->data)->Socket, sentencia, ABORTAR_ESI, id);
    }else{
        sleep(1);
        Enviar_Sentencia(socket, sentencia, EJECUTAR_SENTENCIA_RESP, ID_COORDINADOR);
    }
}
            

void *comunicacion_cli_ser(void *socket_cli){  

    //Get the socket descriptor
    int cliente = *(int *)socket_cli;
    int largo_mensaje;
    Header *headerCoor = malloc(sizeof(Header));
    Sentencia *sentencia;
    //Sentencia *sentencia_ESI = malloc(sizeof(Sentencia));
    int existencia, Clave_en_Instancia, socket_aux;
    Clave_Instancia * clave;
    int retardo = config_get_int_value(config, "Retardo");

    Handshake_Coordinador(cliente);

    //Receive a message from client
    while ((largo_mensaje = recv(cliente, headerCoor, sizeof(Header), 0)) > 0)
    {
        printf("ASUNTO header: %d - Length: %d - IdInstancia: %d\n", headerCoor->Asunto, headerCoor->Length, headerCoor->IdSolicitante);
        switch (headerCoor->Asunto)
        {
        case HANDSHAKE_COORD_INST:
            //valido si la instancia que me habla ya existia
            existencia = Existencia_Instancia(headerCoor->IdSolicitante);
            if(existencia == INSTANCIA_NO_EXISTE || existencia == INSTANCIA_EXISTIA){
                //Cargo la lista de instancias que se conectaron al coordinador
                Cargar_Estructura_Administrativa(cliente, headerCoor->IdSolicitante, INSTANCIA);
                //Si es Key explicit y entra una nueva instancia tengo que redistribuir
                if (algoritmo_distribucion == KE){
                    Algoritmos_Distribucion(NULL, REDISTRIBUCION, CON_EFECTO);
                }
                //Imprimir_Lista(INSTANCIA);
                //Envio las dimensiones de la instancia para que se cree
                enviar_dimensiones_instancia(cliente);
                instancias_a_compactar = Lista_Instancia->elements_count;
                if (existencia == INSTANCIA_EXISTIA){
                    enviar_datos_instancia(cliente);
                }
            }else{
                log_error(logger, "Ya existe instancia con el id:%d\n", headerCoor->IdSolicitante);
                shutdown(cliente, 2);
            }
            break;
        case NUEVO_ESI:
            //Cargo la lista de ESI's que se conectaron al coordinador
            Cargar_Estructura_Administrativa(cliente, headerCoor->IdSolicitante, ESI);
            //Imprimir_Lista(ESI);
            break;
        case HANDSHAKE_PLAN_COORD:
            //Cargo la lista de un solo elemento que contiene al planificador que se conecto al coordinador
            Cargar_Estructura_Administrativa(cliente, headerCoor->IdSolicitante, PLANIFICADOR);
            break;
        case EJECUTAR_SENTENCIA_REQ:
            pthread_mutex_lock(&sem_ejecucion);
            //Recibo la sentencia que viene del ESI
            log_info(logger, "EJECUTAR_SENTENCIA_REQ\n");
            sentencia = Recibir_Sentencia(cliente);
            Clave_en_Instancia = Existencia_Clave(sentencia);
            //printf("Clave_en_Instancia:%d\n", Clave_en_Instancia);
            Resultado_Evaluacion_Coordinador(sentencia, headerCoor->IdSolicitante, Clave_en_Instancia);
            //si libero me rompe por coredump
            //free(sentencia_ESI);
            free(sentencia);
            pthread_mutex_unlock(&sem_ejecucion);
            break;
        case SENTENCIA_PLANIFICADOR_RESP:
            pthread_mutex_lock(&sem_ejecucion);
            //Se recibe la respuesta del planificador respecto a la posibilidad de ejecutar de la sentencia
            log_info(logger, "SENTENCIA_PLANIFICADOR_RESP\n");
            sentencia = Recibir_Sentencia(cliente);
            log_info(logger, "Escribir en instancia:%d\n", Escribir_Instancia);
            
            //printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
            //Se evalua el resultado de la pregunta al planificador respecto a la ejecucion de la sentencia
            Resultado_Evaluacion_Planificador(sentencia, headerCoor->IdSolicitante);
            /* Se evalua si se puede o no ejecutar la sentencia,
                en caso de OK se envia a la instancia asunto: SENTENCIA_INSTANCIA_REQ (ver que mensaje convendria enviarle)
                en caso Bloqueo o Aborto se envia a ESI asunto: EJECUTAR_SENTENCIA_RESP con el resultado que corresponda
                */
               //si libero me rompe por coredump
            //free(sentencia);
            free(sentencia);
            pthread_mutex_unlock(&sem_ejecucion);
            break;
        case SENTENCIA_INSTANCIA_RESP:
            
            //TODO:
            /* Se recibe respuesta de la instancia de si pudo realizar la accion correctamente, se envia a esi asunto: EJECUTAR_SENTENCIA_RESP
                */
            log_info(logger, "SENTENCIA_INSTANCIA_RESP\n");
            sentencia = Recibir_Sentencia(cliente); 
            
            //printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
            socket_aux = Socket_Por_Id(headerCoor->IdSolicitante, ESI);

            //Actualizo claves_instancias

            Clave_en_Instancia = Id_Por_Socket(cliente, INSTANCIA);
            Clave_en_Instancia_GLOBAL = Clave_en_Instancia;

            sentencia_global = sentencia;
            if(!list_any_satisfy(Claves_Instancias, Es_Clave_Existente_En_Instancia)){
                clave = Cargar_Clave(sentencia->Clave, Clave_en_Instancia);
                list_add(Claves_Instancias, clave);
            }
            //PRUEBA
            //Imprimir_Lista_Clave();
            //FIN PRUEBA
            sleep(retardo);
            pthread_mutex_lock(&sem_ejecucion);
            Resultado_Evaluacion_Instancia(sentencia, headerCoor->IdSolicitante, socket_aux);
            //Enviar_Sentencia(socket_aux, sentencia, EJECUTAR_SENTENCIA_RESP, ID_COORDINADOR);
            if((algoritmo_distribucion == LSU) && (sentencia->Operacion == S_SET) && (sentencia->Resultado == OK)){
                Actualizar_Clave_Instancia_LSU(sentencia);
                //Imprimir_Lista(INSTANCIA);
            }
            Loguear_Sentencia(sentencia, headerCoor->IdSolicitante);
            //printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
            free(sentencia);
            pthread_mutex_unlock(&sem_ejecucion);
            break;
            //NO ESTA PROBADO
        case VALOR_CLAVE:
            sentencia = Recibir_Sentencia(cliente);
            Clave_en_Instancia = Id_Por_Clave(sentencia->Clave);
            if(Instancia_Disponible(Clave_en_Instancia) == INSTANCIA_EXISTENTE){
                socket_aux = Socket_Por_Id(Clave_en_Instancia, INSTANCIA);
                Enviar_Sentencia(socket_aux, sentencia, CMD_STATUS, cliente);
            }else{
                strcpy(sentencia->Valor, "Sin Valor");
                Enviar_Sentencia(cliente, sentencia, VALOR_CLAVE, ID_COORDINADOR);
            }
            free(sentencia);
            break;
            //NO ESTA PROBADO
        case CMD_STATUS:
            sentencia = Recibir_Sentencia(cliente);
            Clave_en_Instancia = Id_Por_Clave(sentencia->Clave);
            Enviar_Sentencia(cliente, sentencia, VALOR_CLAVE, Clave_en_Instancia);
            free(sentencia);
            
            break;
        case POSIBLE_INSTANCIA:
            sentencia = Recibir_Sentencia(cliente);
            Algoritmos_Distribucion(sentencia, algoritmo_distribucion, SIMULACION);
            Enviar_Sentencia(cliente,sentencia, POSIBLE_INSTANCIA, Escribir_Instancia);
            free(sentencia);
            
            break;
        case COMPACTAR:
            pthread_mutex_lock(&sem_ejecucion);
            compactando = true;
            Compactar_Instancias_Menos_Socket(cliente);
            break;
        case COMPACTAR_OK:
            log_info(logger, "Cantidad de instancias a compactar:%d\n", instancias_a_compactar);
            pthread_mutex_lock(&sem_compactar);
            instancias_a_compactar--;
            pthread_mutex_unlock(&sem_compactar);
            Semaforos(0, TODOS_MENOS_INSTANCIAS, UNLOCK);
            if(instancias_a_compactar == 0)
                log_info(logger, "Se finalizo la compactacion\n");
                compactando = false;   
            pthread_mutex_unlock(&sem_ejecucion); 
            break;
        default:
        log_error(logger, "Se recibio un asunto incorrecto: %d\n", headerCoor->Asunto);
            break;
        }
        //free(sentencia);
    }
    if (largo_mensaje == 0)
    {
        int index;
        int id_Socket = Id_Por_Socket(cliente, INSTANCIA);
        //Equitative_Load(0, EQUITATIVE);
        if (id_Socket != INSTANCIA_NOT_DIE)
        {
            log_error(logger, "Se desconecto una Instancia.\n");
            index = Index_Por_Socket(cliente, INSTANCIA);
            if(algoritmo_distribucion == KE){
                Redistribuir(id_Socket, algoritmo_distribucion, index);
            }else
                Eliminar_Elemento_Lista(index, INSTANCIA);
                Validar_Compactacion();
        }else{
            id_Socket = Id_Por_Socket(cliente, ESI);
            if(id_Socket != ESI_NOT_DIE){
                log_error(logger, "Se desconecto un ESI.\n");
                index = Index_Por_Socket(cliente, ESI);
                Eliminar_Elemento_Lista(index, ESI);
            }else{
                id_Socket = Id_Por_Socket(cliente, PLANIFICADOR);
                if(id_Socket != PLANIFICADOR_NOT_DIE){
                    log_error(logger, "Se desconecto el Planificador.\n");
                    list_destroy_and_destroy_elements(L_Planificador, free);
                    list_destroy_and_destroy_elements(Lista_Instancia, free);
                    list_destroy_and_destroy_elements(Lista_ESI, free);
                    list_destroy_and_destroy_elements(Claves_Instancias, free);
                    pthread_mutex_destroy(&sem_compactar);
                    pthread_mutex_destroy(&sem_ejecucion);
                    pthread_detach((pthread_t)pthread_self);
                    exit_gracefully(logger, EXIT_SUCCESS);
                    
                    return 0;
                }else{
                     log_error(logger, "Se rechazo cliente por ID duplicado.\n");
                }
            }

        }
    }
    else if (largo_mensaje == -1)
    {
        
        
        log_error(logger, "Fallo la recepcion del mensaje.\n");
    }
    free(headerCoor);
    return 0;
}



int main(void)
{


    // -- Leemos archivo de configuracion
    Crear_Log_Operaciones();
    Crear_Log_();

    config = config_create("config.txt");
    if (config != NULL && !dictionary_is_empty(config->properties))
    {
        log_info(logger, "Las configuraciones fueron cargadas satisfactoriamente.\n");
    }
    else {
        log_error(logger, "Las configuraciones no fueron cargadas.\n");
    }


    // -- Levantamos el servidor
    struct sockaddr_in direccionServidor;
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_addr.s_addr = INADDR_ANY;
    direccionServidor.sin_port = htons(config_get_int_value(config, "Puerto_Escucha"));

    int servidor = socket(AF_INET, SOCK_STREAM, 0);

    int activado = 1;
    setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

    if (bind(servidor, (void *)&direccionServidor, sizeof(direccionServidor)) != 0)
    {
        log_error(logger, "Falló el bind.\n");
        return 1;
    }
    char *algoritmo = config_get_string_value(config, "Algoritmo_Distribucion");
    Algoritmo_Distribucion(algoritmo);
    Lista_Instancia = list_create();
    Lista_ESI = list_create();
    L_Planificador = list_create();
    Claves_Instancias = list_create();
    pthread_mutex_init(&sem_compactar, NULL);
    pthread_mutex_init(&sem_ejecucion, NULL);
    /*
    Clave_Instancia *Clave1;
    Clave_Instancia *Clave2;
    Clave_Instancia *Clave3;
    Clave_Instancia *Clave4;
    Clave_Instancia *Clave5;
    Clave_Instancia *Clave6;
    Clave1 = Cargar_Clave("gregui",1);
    Clave2 = Cargar_Clave("emi",1);
    Clave3 = Cargar_Clave("pato",2);
    Clave4 = Cargar_Clave("lucas",4);
    Clave5 = Cargar_Clave("brian",3);
    Clave6 = Cargar_Clave("tu mama",2);
    list_add(Claves_Instancias, Clave1);
    list_add(Claves_Instancias, Clave2);
    list_add(Claves_Instancias, Clave3);
    list_add(Claves_Instancias, Clave4);
    list_add(Claves_Instancias, Clave5);
    list_add(Claves_Instancias, Clave6);
    Imprimir_Lista_Clave();
    */
    log_info(logger, "Estoy escuchando.\n");
    int resListen = listen(servidor,100);
    if(resListen < 0) {
        log_error(logger, "No estoy escuchando.\n");
        return 1;
    }

    // Esperamos que se conecte un cliente
    struct sockaddr_in direccionCliente;
    int tamanioDireccion = sizeof(struct sockaddr_in);

    pthread_t thread_id;
    int cliente;

    while ((cliente = accept(servidor, (struct sockaddr *)&direccionCliente, (socklen_t *)&tamanioDireccion)))
    {
        
        log_info(logger, "Conexion Aceptada.\n");

        if (pthread_create(&thread_id, NULL, comunicacion_cli_ser, (void *)&cliente) < 0)
        {
            log_error(logger, "No se pudo crear el Hilo.\n");
            return 1;
        }
        log_info(logger, "Hilo Asignado.\n");    
    }

    if (cliente < 0)
    {
        log_error(logger, "Fallo la aceptacion de conexion.\n");
        return 1;
    }
    
    return 0;

   
}

bool Es_Clave_Existente_En_Instancia(void * claveInstancia){

    return (0 == strcmp(sentencia_global->Clave, ((Clave_Instancia*)claveInstancia)->Clave)) && Clave_en_Instancia_GLOBAL == ((Clave_Instancia*)claveInstancia)->IdInstancia;
}
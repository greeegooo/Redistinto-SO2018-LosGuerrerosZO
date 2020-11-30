#include "../includes/run.h"

RESULTADO Ejecutar_Operacion(Instancia ** instancia, Sentencia * sentencia);
RESULTADO get(Instancia ** instancia, Sentencia * sentencia);
RESULTADO set(Instancia ** instancia, Sentencia * sentencia);
RESULTADO store(Instancia ** instancia, Sentencia * sentencia);

RESULTADO circular(Instancia ** instancia, Sentencia * sentencia);
RESULTADO leastRecentlyUsed(Instancia ** instancia, Sentencia * sentencia);
RESULTADO biggestSpaceUsed(Instancia ** instancia, Sentencia * sentencia);

t_list * BuscarVictimas(t_list* lista, bool(*condicion)(void*));
void SetearPrimeraVictima(Instancia ** instancia);
bool HayEntradasAtomicas(Instancia ** instancia);
int AsignarNuevaVictima(Instancia ** instancia);
bool ExisteLaClave(Instancia ** instancia, char * clave);
RESULTADO Actualizar(Instancia ** instancia, Sentencia * sentencia);

//Closures
bool _esVictimaParaCircular(void * entrada);
bool _existeClave(void * entrada);
bool _existeClave_Compactar(void * clave);
bool _esAtomica(void * entrada);
bool _bsu_comparator(void * e1, void * e2);
bool _lru_comparator(void * e1, void * e2);
//Fin Closures

bool SePuedeActualizar(Instancia ** instancia, Sentencia * sentencia);
void LoggearSentencia(Sentencia * sentencia, RESULTADO res);
bool HaySuficienteEspacio(Instancia ** instancia, Sentencia * sentencia);
void AvisarCompactacion(Instancia ** instancia);
void Compactar(Instancia ** instancia);
void CompactacionOK(Instancia ** instancia);

void Timer(void);

void ActualizarReferenciado(Sentencia * sentencia);
void ActualizarEspacioUsado(Sentencia * sentencia);

void run (Instancia ** instancia) {

    int result;
    Header * header_Coor = Crear_Header_Vacio();

    //Iniciar Dump Timer

    instanciaAux = (*instancia);
    
    pthread_t dump;
    pthread_create(&dump, NULL, (void *) Timer, NULL);

    while((result = recv((*instancia)->socket , header_Coor , sizeof(Header) , 0)) > 0) {

        //printf("ASUNTO header: %d\n Length: %d\n IdInstancia: %d\n", header_Coor->Asunto, header_Coor->Length, header_Coor->IdSolicitante);
        switch(header_Coor->Asunto) {

            case CLAVES_EXISTENTES: {

                //Recibir Sentencia, pero solo leer la clave para saber que buscar en el dump
                Sentencia * sentencia = Recibir_Sentencia((*instancia)->socket);

                Levantar_Archivo(instancia, sentencia);
                
                break;
            }
            case SENTENCIA_INSTANCIA_REQ: {

                Sentencia * sentencia = Recibir_Sentencia((*instancia)->socket);
                //printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
                sentencia->Resultado = Ejecutar_Operacion(instancia, sentencia);
                //printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
                Enviar_Sentencia((*instancia)->socket, sentencia, SENTENCIA_INSTANCIA_RESP, header_Coor->IdSolicitante);

                break;
            }
            case CMD_STATUS:{
                //ESTAMOS CONSIDERANDO QUE EL COORDINADOR TIENE ACTUALIZADAS LAS CLAVES QUE YO TENGO
                char * valor = NULL;
                Sentencia * sentencia = Recibir_Sentencia((*instancia)->socket);
                valor = obtener_valor_tabla(instancia, sentencia);
                strcpy(sentencia->Valor, valor);
                Enviar_Sentencia((*instancia)->socket, sentencia, CMD_STATUS, (*instancia)->id);
                break;
            }
            case COMPACTAR:{
                //printf("Si ES NOS VAMOS DE PUTAS\n");
                Compactar(instancia);
                break;
            }
            default:
                break;
        }
        
		memset(header_Coor, 0, sizeof(Header));
    }

    free(header_Coor);

    if(result == CLIENTE_DESCONECTADO) _exit_with_error(logger, (*instancia)->socket, "Se desconecto el Coordinador", NULL);
    if(result == ERROR_RECV) _exit_with_error(logger, (*instancia)->socket, "Fallo la recepcion del mensaje", NULL);
}

char * obtener_path_dump(Instancia ** instancia, char * clave){

    char * path = malloc(sizeof(char)*TAMANIO_MAX_VALOR);

    strcpy(path, "");
    strcat(path, (*instancia)->montaje);
    strcat(path, clave);
    strcat(path, EXTENSION_ARCHIVO);

    return path;
}

void Timer(void){

    sleep(instanciaAux->intervalo);
    Dump(instanciaAux);
    Timer();
}

void Dump(Instancia * instancia){
    
    int cantidadEntradas = instancia->cantidad_Entradas;
    t_entrada * entrada = NULL;
    int entradasNecesarias = 0;
    char * valor = NULL;


    for(int i = 0; i<cantidadEntradas; i++){

        entrada = (t_entrada *) list_get(instancia->tabla_Entradas, i);
        
        if(entrada->ocupado){
            
            char * path = obtener_path_dump(&instancia, entrada->clave);

            valor = malloc(sizeof(char) * entrada->tamanioValor);

            memcpy(valor, entrada->ptrEntrada, entrada->tamanioValor);

            entradasNecesarias = Entradas_necesarias(valor, instancia->tamanio_Entrada);

            if (escribir_en_disco(path, valor)){

                log_info(logger, "Se escribio al clave %s en el disco", entrada->clave);
            }
            else {

                log_error(logger, "No se escribio la clave %s en el disco", entrada->clave);
            }

            i = i + entradasNecesarias - 1;

            free(path);
            free(valor);
        }
    }

    log_info(logger, "Dump terminado");
}

RESULTADO Ejecutar_Operacion(Instancia ** instancia, Sentencia * sentencia) {

    RESULTADO res;

    switch(sentencia->Operacion){
        case S_GET: res = get(instancia, sentencia);
            break;
        case S_SET: res = set(instancia, sentencia);
            break;
        case S_STORE: res = store(instancia, sentencia);
            break;  
    }

    return res;
}

RESULTADO get(Instancia ** instancia, Sentencia * sentencia) {

    RESULTADO res = OK;

    LoggearSentencia(sentencia, res);
    
    return res;
};

void LoggearSentencia(Sentencia * sentencia, RESULTADO res){

    char * msg = malloc(sizeof(char)*100);
    char * operacion = Traducir_Operacion(sentencia->Operacion);
    char * resultado = Traducir_Resultado(res);
    sprintf(msg, "Op:%s | Key:%s | Val:%s | Res:%s", operacion, sentencia->Clave, sentencia->Valor, resultado);
    log_info(logger, msg);
    free(msg);
    free(operacion);
    free(resultado);
}

RESULTADO set(Instancia ** instancia, Sentencia * sentencia) {

    RESULTADO res;
    t_link_element * entrada = NULL;

    bool existeClave = ExisteLaClave(instancia, sentencia->Clave);

    //printf("CLAVE SET %d\n", existeClave);

    if(existeClave){

        log_info(logger, "set.Actualizar: Clave existente");
        res = Actualizar(instancia, sentencia);
    }
    else{
        //printf("Cantidad %d\n", (*instancia)->cantidad_Entradas);
        log_info(logger, "set.ObtenerMemoriaContiguaLibrePara: Clave inexistente");
        entrada = ObtenerMemoriaContiguaLibrePara(instancia, sentencia);

        if(entrada != NULL) {
            //printf("Cantidad %d\n", (*instancia)->cantidad_Entradas);
            log_info(logger, "set.Escribir: Encontramos memoria libre");
            res = Escribir(entrada, sentencia);
        }
        else { 

            if(HaySuficienteEspacio(instancia, sentencia)){
                //printf("Cantidad %d\n", (*instancia)->cantidad_Entradas);
                log_info(logger, "set.AvisarCompactacion");
                AvisarCompactacion(instancia);

                //imprimir_memoria(instancia);
                log_info(logger, "set.Compactar");
                //printf("Cantidad %d\n", (*instancia)->cantidad_Entradas);
                Compactar(instancia); 

                //printf("NUEVA INSTANCIA LPM\n");
                //imprimir_memoria(instancia);
                log_info(logger, "set.ObtenerMemoriaContiguaLibrePara: post-Compactacion");
                log_info(logger, "Entradas necesarias %d", Entradas_necesarias(sentencia->Valor, (*instancia)->tamanio_Entrada));

                entrada = ObtenerMemoriaContiguaLibrePara(instancia, sentencia);

                //printf("Entrada \n", entrada->);
                if(entrada != NULL) {
                    log_info(logger, "set.Escribir: Encontramos memoria libre: post-Compactacion");
                    res = Escribir(entrada, sentencia);
                }
                else {
                    log_info(logger, "set.ObtenerMemoriaContiguaLibrePara: No Encontramos memoria libre: post-Compactacion");
                }  
            }
            else{

                log_info(logger, "set.Reemplazar");
                res = Reemplazar(instancia, sentencia);
            }            
        }
    }

    imprimir_memoria(instancia);
    LoggearSentencia(sentencia, res);

    return res;
}

bool HaySuficienteEspacio(Instancia ** instancia, Sentencia * sentencia){

    int cantLibres = list_filter((*instancia)->tabla_Entradas, _es_libre)->elements_count;
    int cantNecesaria = Entradas_necesarias(sentencia->Valor, (*instancia)->tamanio_Entrada);

    return cantLibres >= cantNecesaria;
}

RESULTADO store(Instancia ** instancia, Sentencia * sentencia){

    RESULTADO res = OK;

    LoggearSentencia(sentencia, res);
    char * valor = obtener_valor_tabla(instancia, sentencia);
    char * path = obtener_path(instancia, sentencia);
    if (escribir_en_disco(path, valor)){
        log_info(logger, "Se escribio la clave %s en el disco", sentencia->Clave);
    }
    else {
        log_error(logger, "No se logró escribir ls clave %s en el disco", sentencia->Clave);
    }

    free(path);
    free(valor);

    return res;
};

bool escribir_en_disco(char * path, char * valor){

    FILE * archivo = NULL;
    archivo = fopen(path, "w");

    if(archivo==NULL){

        return false;
    }

    char * aux = strcat(valor, "");

    if(strlen(aux)>0){
        fwrite(aux, sizeof(char), strlen(aux), archivo);
    }
    else{
        fwrite(TOKEN_FIN_TEXTO, sizeof(char), strlen(aux), archivo);
    }

    fclose(archivo);

    return true;
}

char * obtener_path(Instancia ** instancia, Sentencia * sentencia){

    char * path = malloc(sizeof(char)*TAMANIO_MAX_VALOR);

    strcpy(path, "");
    strcat(path, (*instancia)->montaje);
    strcat(path, sentencia->Clave);
    strcat(path, EXTENSION_ARCHIVO);

    return path;
}

char * obtener_valor_tabla(Instancia ** instancia, Sentencia * sentencia){


    int nroEntradas = (*instancia)->cantidad_Entradas;
    t_entrada * entrada = NULL;
    
    char * valor = malloc(sizeof(char) * TAMANIO_MAX_VALOR);
    memset(valor, 0, TAMANIO_MAX_VALOR);

    //printf("valor %s", valor);
    for(int i = 0;i <nroEntradas; i++){
        entrada = (t_entrada *) list_get((*instancia)->tabla_Entradas, i);
        if((strcmp(sentencia->Clave, entrada->clave))==0){
            //printf("valorantes %s", valor);
            memcpy(valor, entrada->ptrEntrada, entrada->tamanioValor);
            //printf("valordespiues %s", valor);
            i = nroEntradas;
        }
    }
    
    //printf("Valor %s\n", valor);
    return valor;
}

char * obtener_valor_archivo(Instancia ** instancia, Sentencia * sentencia){
    
    char * path = obtener_path(instancia, sentencia);
    char * valor = malloc(sizeof(char)*TAMANIO_MAX_VALOR);
    size_t cantidadCaracteres = 0;
    //strcpy(valor,"");
    FILE * archivo = NULL;
    archivo = fopen(path, "r");
    if(archivo==NULL){

        free(path);
        return NULL;
    }
    else{
        fseek(archivo, 0L, SEEK_END);
        cantidadCaracteres = (size_t) ftell(archivo);
        //printf("cantidad %d", cantidadCaracteres);
        fseek(archivo, 0L, SEEK_SET);
        fread(valor, sizeof(char), cantidadCaracteres, archivo);
        //printf("valor archivo %s\n", valor);
        strcpy(valor+cantidadCaracteres, TOKEN_FIN_TEXTO);
        //printf("valor archivo %s\n", valor);
        fclose(archivo);

        free(path);
        return valor;
    }
}

void Levantar_Archivo(Instancia ** instancia, Sentencia * sentencia) {

    char * valor = obtener_valor_archivo(instancia, sentencia);

    if(valor == NULL || strlen(valor) < 1){
        Header * headerABORTAR = Crear_Header(CLAVES_EXISTENTES_ABORTAR, SIN_TAMANIO, (*instancia)->id);
        write((*instancia)->socket, headerABORTAR, sizeof(Header));
        free(headerABORTAR);
    }
    else{
        strcpy(sentencia->Valor, valor);
        t_link_element * entrada = NULL;
        entrada = ObtenerMemoriaContiguaLibrePara(instancia, sentencia);
        Escribir(entrada, sentencia);
        Header * headerOK = Crear_Header(CLAVES_EXISTENTES_OK, SIN_TAMANIO, (*instancia)->id);
        write((*instancia)->socket, headerOK, sizeof(Header));
        free(headerOK);
    }

    free(valor);

    imprimir_memoria(instancia);
}

RESULTADO Escribir(t_link_element * entrada, Sentencia * sentencia){

    RESULTADO res = OK;
    int cantEntradas = Entradas_necesarias(sentencia->Valor, ((t_entrada*)entrada->data)->tamanio);

    strcpy(((t_entrada*)entrada->data)->ptrEntrada, sentencia->Valor);
    //if(ret == NULL) res = ABORTAR;

    for(int i = 0; i < cantEntradas; i++) {
    
        //hay que actualizar las entradas correspondientes
        strcpy(((t_entrada*)entrada->data)->clave, sentencia->Clave);
        ((t_entrada*)entrada->data)->ocupado = true;
        ((t_entrada*)entrada->data)->esAtomico = cantEntradas == 1;
        ((t_entrada*)entrada->data)->entradasOcupadas = cantEntradas;
        ((t_entrada*)entrada->data)->tamanioValor = strlen(sentencia->Valor);
        //if(cantEntradas > 1) ((t_entrada*)entrada->data)->esVictima = false;

        entrada = entrada->next;
    }
    
    ActualizarReferenciado(sentencia);
    ActualizarEspacioUsado(sentencia);

    return res;
}

void AvisarCompactacion(Instancia ** instancia){

    Header * header = Crear_Header(COMPACTAR, SIN_PAYLOAD, (*instancia)->id);
    void * buf = malloc(sizeof(Header));
    memcpy(buf, header, sizeof(Header));
    write((*instancia)->socket , header , sizeof(Header));

    free(header);
    free(buf);
}

void Compactar(Instancia ** instancia){

    //enviar mensajr ASUNTO COMPACTAR
    //sleep(10);
    log_info(logger, "Compactar: Entramos en el proceso de compactacion");

    //Crear una tabla de entradas paralela 
    Instancia * instanciaNueva = crear_instancia();
    t_list * clavesYaInsertadas = list_create();
    bool claveExiste = false;
    //Creamos una instancia nueva
    instanciaNueva->cantidad_Entradas = (*instancia)->cantidad_Entradas;
    instanciaNueva->tamanio_Entrada = (*instancia)->tamanio_Memoria;
    //instanciaNueva->
    instanciaNueva->memoria = malloc(sizeof(char) * (*instancia)->tamanio_Memoria);
    memset((void*)instanciaNueva->memoria, 0, (*instancia)->tamanio_Memoria);

    for(int i = 0; i < (*instancia)->cantidad_Entradas; i++) {
        
        int id = i + 1;
        char * ptrEntrada = instanciaNueva->memoria + (id - 1) * (*instancia)->tamanio_Entrada;
        
        t_entrada * entrada = Crear_Entrada(id, (*instancia)->tamanio_Entrada, ptrEntrada);
        
        list_add(instanciaNueva->tabla_Entradas, entrada);
    }   

    //printf("Cantidad %d\n", (*instancia)->cantidad_Entradas);
    //Empezamos a llenar los valores en la nueva instancia
    t_link_element * aux = (*instancia)->tabla_Entradas->head;
    //t_link_element * libre = NULL;

    for(int i = 0; i < (*instancia)->cantidad_Entradas; i++){

        //Me fijo si la clave de esta entrada ya fue tomada o si es una entrada vacia
        t_entrada * entrada_aux = (t_entrada*)aux->data;
        clave_aux_existeClave_Compactar = entrada_aux->clave;
        //printf("Clave %s\n", clave_aux_existeClave_Compactar);

        if(strcmp(clave_aux_existeClave_Compactar, "") != 0){

            claveExiste = list_any_satisfy(clavesYaInsertadas,_existeClave_Compactar);
        }
        else
        {
            claveExiste = true;
        }
        
        if(!claveExiste){

            Sentencia * sentencia = Crear_MockSentencia(S_SET, ((t_entrada*)aux->data)->clave, "");
            char * valor = obtener_valor_tabla(instancia, sentencia);
            strcpy(sentencia->Valor,valor);

            //deberia setear sin problemas
            t_link_element * element = ObtenerMemoriaContiguaLibrePara(&instanciaNueva, sentencia);
            Escribir(element, sentencia);
            //set(&instanciaNueva, sentencia);

            list_add(clavesYaInsertadas, ((t_entrada*)aux->data)->clave);
        }

        //printf("Cantidad de claves insertadas %d\n",list_size(clavesYaInsertadas));
        //printf("Nueva Instancia\n");
        //imprimir_memoria(&instanciaNueva);
        aux = aux->next;
    }

    //printf("4\n");

    //FOTO DE PRUEBA
    //printf("Instancia\n");
    //imprimir_memoria(instancia);

    //printf("Instancia NUeva\n");
    //imprimir_memoria(&instanciaNueva);
    //FIN PRUEBA

    //Le robamos la tabla de entrada a la instancia nueva

    (*instancia)->memoria = NULL;
    (*instancia)->memoria = instanciaNueva->memoria;

    memcpy((*instancia)->memoria, instanciaNueva->memoria, (*instancia)->tamanio_Memoria);

    list_destroy((*instancia)->tabla_Entradas); 
    (*instancia)->tabla_Entradas = list_create();

    list_add_all((*instancia)->tabla_Entradas, instanciaNueva->tabla_Entradas);

    //printf("Instancia compactada\n");
    //imprimir_memoria(instancia);

    CompactacionOK(instancia);
} 

void CompactacionOK(Instancia ** instancia){

    Header * header = Crear_Header(COMPACTAR_OK, SIN_PAYLOAD, (*instancia)->id);
    void * buf = malloc(sizeof(Header));
    memcpy(buf, header, sizeof(Header));
    write((*instancia)->socket , header , sizeof(Header));

    free(header);
    free(buf);
    log_info(logger, "Compactar: Compactacion exitosa");
}

RESULTADO Reemplazar(Instancia ** instancia, Sentencia * sentencia){

    RESULTADO res = OK;

    if(Entradas_necesarias(sentencia->Valor, (*instancia)->tamanio_Entrada) > 1) {

        res = ABORTAR;
        log_info(logger, "El valor a reemplazar es mayor que una entrada.");
    }
    else{

        switch((*instancia)->algoritmo){
            case CIRC: res = circular(instancia, sentencia);
                break;
            case LRU: res = leastRecentlyUsed(instancia, sentencia);
                break;
            case BSU: res = biggestSpaceUsed(instancia, sentencia);
                break;
            default:
                break;
        }
    }
    
    return res;
}

RESULTADO Actualizar(Instancia ** instancia, Sentencia * sentencia){
    
    log_info(logger, "Actualizar: Entramos a actualizar");
    RESULTADO res = OK;
    t_link_element * aux = (*instancia)->tabla_Entradas->head;
    t_link_element * entradaActualizar = NULL;
    bool esPrimeraEntrada = true;

    //Antes de actualizar tengo que validar si las entradas a reemplazar no son mas que las que ya estan asignadas
    if(!SePuedeActualizar(instancia, sentencia)) {

        //printf("1\n");
        res = ABORTAR;
        LoggearSentencia(sentencia, res);
        return res;
    }

    //printf("Cantidad %d\n", (*instancia)->cantidad_Entradas);
    for(int i = 0; i < (*instancia)->cantidad_Entradas; i++){

        if(strcmp(((t_entrada*)aux->data)->clave, sentencia->Clave) == 0) {

            if(esPrimeraEntrada) {
                entradaActualizar = aux;
                esPrimeraEntrada = false;
            }
            
            memset(((t_entrada*)aux->data)->ptrEntrada, 0, (*instancia)->tamanio_Entrada);
            strcpy(((t_entrada*)aux->data)->clave, "");
            ((t_entrada*)aux->data)->ocupado = false;
            ((t_entrada*)aux->data)->esAtomico = true;
            ((t_entrada*)aux->data)->entradasOcupadas = 0;
        }

        aux = aux->next;
    }
    
    res = Escribir(entradaActualizar, sentencia);

    return res;
}

bool SePuedeActualizar(Instancia ** instancia, Sentencia * sentencia){

    //Necesito saber cuantas entradas ocupa el valor actual
    t_link_element * aux = NULL;
    aux = (*instancia)->tabla_Entradas->head;
    //bool ret = false;
    bool encontramosEntrada = false;
    int entradasOcupadas = 0;

    while(!encontramosEntrada && aux->next != NULL){

        if(strcmp(((t_entrada*)aux->data)->clave,sentencia->Clave) == 0){
            
            entradasOcupadas = ((t_entrada*)aux->data)->entradasOcupadas;
            encontramosEntrada = true;
        } 
        aux = aux->next;
    }


    //Comparar contra las entradas 
    return entradasOcupadas >= Entradas_necesarias(sentencia->Valor, (*instancia)->tamanio_Entrada);
}

RESULTADO circular(Instancia ** instancia, Sentencia * sentencia){
    
    RESULTADO res = OK;
    t_list * victimas = NULL;

    log_info(logger, "Reemplazando con algoritmo circular");
    //Busco la victima, en CIRC solo puede ser 1
    victimas = BuscarVictimas((*instancia)->tabla_Entradas, _esVictimaParaCircular);
    //printf("cant victimas %d\n", victimas->elements_count);

    if(victimas->elements_count < 1){

        if(HayEntradasAtomicas(instancia)){//Entonces nunca se seteo ninguna victima
        
            SetearPrimeraVictima(instancia);
            victimas = NULL;
            victimas = BuscarVictimas((*instancia)->tabla_Entradas, _esVictimaParaCircular);

            memset(((t_entrada*)victimas->head->data)->ptrEntrada, 0, (*instancia)->tamanio_Entrada);
            res = Escribir(victimas->head, sentencia);
            AsignarNuevaVictima(instancia);
        }
        else{
            res = ABORTAR;
            LoggearSentencia(sentencia, res);
            return res;
        }
    } 
    else{
        memset(((t_entrada*)victimas->head->data)->ptrEntrada, 0, (*instancia)->tamanio_Entrada);
        res = Escribir(victimas->head, sentencia);
        AsignarNuevaVictima(instancia);
    }

    list_destroy(victimas);

    return res;
}

void SetearPrimeraVictima(Instancia ** instancia){  

    t_list * list = (*instancia)->tabla_Entradas;
    ((t_entrada*)list_filter(list, _esAtomica)->head->data)->esVictima = true;
}

RESULTADO leastRecentlyUsed(Instancia ** instancia, Sentencia * sentencia){
    
    RESULTADO res = OK;

    //Busco el que mayor valor referenciado
    //memset en 0 para tamanio de entrada
    //Escribir()

    //t_list * victimas = NULL;
    bool hayEmpate = false;
    log_info(logger, "Reemplazando con algoritmo LRU");

    t_list * sortedList = list_filter(list_duplicate((*instancia)->tabla_Entradas), _esAtomica);
    list_sort(sortedList, _lru_comparator);

    t_entrada * sortedHead = (t_entrada*)sortedList->head->data;

    printf("Cantidad %d\n", sortedList->elements_count);
    printf("Cabeza de ref %s\n", sortedHead->clave);
    if(sortedList->elements_count < 2){

        memset(sortedHead->ptrEntrada, 0, (*instancia)->tamanio_Entrada);
        res = Escribir(sortedList->head, sentencia);
    }
    else{

        printf("ENTRAMOS AL ELSE\n");
        for(int i = 0; i < sortedList->elements_count; i++){

            t_entrada * aux = (t_entrada*)list_get(sortedList, i);

            if(strcmp(sortedHead->clave, aux->clave) != 0) {

                hayEmpate = sortedHead->referenciado == aux->referenciado;
            }
        }
    }
    

    if(hayEmpate) res = circular(instancia, sentencia);
    else{

        memset(sortedHead->ptrEntrada, 0, (*instancia)->tamanio_Entrada);
        res = Escribir(sortedList->head, sentencia);
    }

    list_destroy(sortedList);

    return res;
}

bool _lru_comparator(void * e1, void * e2){

    return ((t_entrada*)e1)->referenciado > ((t_entrada*)e2)->referenciado;
}

bool _bsu_comparator(void * e1, void * e2){

    return ((t_entrada*)e1)->bytesOcupados > ((t_entrada*)e2)->bytesOcupados;
}

void ActualizarReferenciado(Sentencia * sentencia){

    //printf("Entramos a ActualizarReferenciado\n");
    t_entrada * aux = NULL;
    //printf("cantidad entradas global %d\n", instanciaAux->cantidad_Entradas);

    for(int i = 0; i < instanciaAux->cantidad_Entradas; i++){
        
        aux = list_get(instanciaAux->tabla_Entradas, i);

        if(aux->esAtomico){
            
            aux->referenciado++;

            if(strcmp(aux->clave, sentencia->Clave) == 0){

                aux->referenciado = 0;
            }
        }
    }
}

void ActualizarEspacioUsado(Sentencia * sentencia){

    //printf("Entramos a ActualizarEspacioUsado\n");
    t_entrada * aux = NULL;
    //char * valorEntrada = malloc(sizeof(char) * instanciaAux->tamanio_Entrada);
    //printf("cantidad entradas global %d\n", instanciaAux->cantidad_Entradas);

    for(int i = 0; i < instanciaAux->cantidad_Entradas; i++){
        
        aux = list_get(instanciaAux->tabla_Entradas, i);

        if(aux->esAtomico){
            
            //memcpy(valorEntrada, aux->ptrEntrada, aux->tamanioValor);

            //aux->bytesOcupados = strlen(valorEntrada) >= aux->tamanio ? aux->tamanio :strlen(aux->ptrEntrada);
            aux->bytesOcupados = aux->tamanioValor;
        }
    }

    //free(valorEntrada);
}

RESULTADO biggestSpaceUsed(Instancia ** instancia, Sentencia * sentencia){
    
    RESULTADO res = OK;

    //t_list * victimas = NULL;
    bool hayEmpate = false;
    log_info(logger, "Reemplazando con algoritmo BSU");

    t_list * sortedList = list_duplicate((*instancia)->tabla_Entradas);
    list_sort(sortedList, _bsu_comparator);

    t_entrada * sortedHead = (t_entrada*)sortedList->head->data;

    for(int i = 1; i < sortedList->elements_count; i++){

        t_entrada * aux = (t_entrada*)list_get(sortedList, i);

        if(strcmp(sortedHead->clave, aux->clave) != 0) {

            hayEmpate = sortedHead->bytesOcupados == aux->bytesOcupados;
        }
    }

    if(hayEmpate) res = circular(instancia, sentencia);
    else{

        memset(sortedHead->ptrEntrada, 0, (*instancia)->tamanio_Entrada);
        res = Escribir(sortedList->head, sentencia);
    }


    list_destroy(sortedList);

    return res;
}

t_list * BuscarVictimas(t_list* lista, bool(*condicion)(void*)){

    return list_filter(lista, condicion);
}

bool HayEntradasAtomicas(Instancia ** instancia){
    return list_any_satisfy((*instancia)->tabla_Entradas,_esAtomica);
}

bool _esAtomica(void * entrada){
    return ((t_entrada*)entrada)->esAtomico;
}
bool _esVictimaParaCircular(void * entrada){

    return ((t_entrada*)entrada)->esVictima && ((t_entrada*)entrada)->esAtomico;
}

int AsignarNuevaVictima(Instancia ** instancia){

    t_link_element * aux = NULL;
    t_link_element * victimaActual = NULL;
    bool victimaSeteada = false;
    bool terminoElCiclo = false;
    
    aux = (*instancia)->tabla_Entradas->head;

    //Encuentro la victima actual
    for(int i = 0; i < (*instancia)->cantidad_Entradas; i++){

        if(((t_entrada*)aux->data)->esVictima) victimaActual = aux;   
        else aux = aux->next;
    }

    //Si no encontre nada devuelvo -1
    if(victimaActual == NULL) {
        log_info(logger, "AsignarNuevaVictima: No se encontro víctima actual en Tabla de Entradas");
        return -1;
    }
    
    //Pregunto por el siguiente desde victimaActual hasta terminar
    aux = NULL;
    aux = victimaActual;

    while(!victimaSeteada && aux->next != NULL){
    //while(!victimaSeteada && aux != NULL){
        //Si es atomico y no es victima, lo seteo como victima
        if(((t_entrada*)aux->next->data)->esAtomico 
           && !(((t_entrada*)aux->next->data)->esVictima)){
               
            ((t_entrada*)aux->next->data)->esVictima = true;
            ((t_entrada*)victimaActual->data)->esVictima = false;

            victimaSeteada = true;
        }

        aux = aux->next;
        
    }

    if(victimaSeteada) {

        log_info(logger, "AsignarNuevaVictima: Se asignó nueva víctima en Tabla de Entradas");
        return 0;
    }

    aux = NULL;
    aux = (*instancia)->tabla_Entradas->head;
    //Sino no encontre nada pregunto desde head hasta que sea igual a victima
    while(!victimaSeteada && !terminoElCiclo){
        
        if(((t_entrada*)aux->data)->esAtomico 
           && !(((t_entrada*)aux->data)->esVictima)){

            ((t_entrada*)aux->data)->esVictima = true;
            ((t_entrada*)victimaActual->data)->esVictima = false;

            victimaSeteada = true;
        }

        if(aux == victimaActual) terminoElCiclo = true;
        aux = aux->next;

    }

    //Si victima no esta seteada entonces todos los valores son no atomicos
    if(victimaSeteada == 0) {
        
        log_info(logger, "AsignarNuevaVictima: No se pudo asignar nueva víctima en Tabla de Entradas. Todos los valores son NO atómicos.");
        return -1;
    }
    
    log_info(logger, "AsignarNuevaVictima: Se asignó nueva víctima en Tabla de Entradas.");

    return 0;
        
}

bool ExisteLaClave(Instancia ** instancia, char * clave){

    clave_aux_existeClave = malloc((sizeof(char) * strlen (clave)) + 1);
    strcpy(clave_aux_existeClave, clave);

    ///printf("ExisteLaClave 1 %s\n", clave);
    //printf("EntramExisteLaClaveos 2 %s\n", clave_aux_existeClave);

    bool ret = list_any_satisfy((*instancia)->tabla_Entradas, _existeClave);

    free(clave_aux_existeClave);
    
    return ret;
};

bool _existeClave(void * entrada){

    //printf("_existeClave 1 %s\n", ((t_entrada*)entrada)->clave);
    //printf("_existeClave 2 %s\n", clave_aux_existeClave);

    return strcmp(((t_entrada*)entrada)->clave, clave_aux_existeClave) == 0;
}

bool _existeClave_Compactar(void * clave){
    
    return strcmp((char*)clave, clave_aux_existeClave_Compactar) == 0;
}
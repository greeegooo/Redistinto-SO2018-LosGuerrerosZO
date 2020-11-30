#include "../includes/entrada.h"

t_entrada * Crear_Entrada(int id, int tamanio, void * ptrEntrada) {

    t_entrada * entrada = malloc(sizeof(t_entrada));

    strcpy(entrada->clave, "");
    entrada->ocupado = false;
    entrada->esAtomico = true;
    entrada->id = id;
    entrada->tamanio = tamanio;
    entrada->ptrEntrada = ptrEntrada;
    entrada->bytesOcupados = 0;
    entrada->esVictima = false;
    entrada->referenciado = 0;
    entrada->entradasOcupadas = 0;
    entrada->tamanioValor = 0;
    return entrada;
}

void Liberar_Entrada(t_entrada ** entrada) {

    free((*entrada)->clave);
    free((*entrada)->ptrEntrada);
    free(entrada);
}

int Entradas_necesarias(char * valor, int tamanio) {

    return (int)ceil((float)strlen(valor) / tamanio);
}


bool Es_entrada_libre(t_entrada * entrada){

    return !entrada->ocupado;
}

bool _es_libre(void * entrada){
    return !((t_entrada*)entrada)->ocupado;
}

t_entrada * Obtener_entrada(t_list * tabla, int index) {

    return (t_entrada *)list_get(tabla, index);
}

/*
bool Hay_entradas_libres(Instancia ** instancia) {

    return list_any_satisfy((*instancia)->tabla_Entradas, Es_entrada_libre);
}

t_list * Entradas_libres(Instancia ** instancia) {

    return list_filter((*instancia)->tabla_Entradas, Es_entrada_libre);
}
*/

bool Son_contiguas(t_entrada * anterior, t_entrada * siguiente) {

    return anterior->ptrEntrada + anterior->tamanio == siguiente->ptrEntrada;
}

t_link_element * ObtenerMemoriaContiguaLibrePara(Instancia ** instancia, Sentencia * sentencia) {

    //printf("Entramos Obtener\n");
    t_list * tablaEntradas = (*instancia)->tabla_Entradas;
    t_link_element * aux = tablaEntradas->head;
    t_link_element * candidato = aux;
    int contador = 0;
    int cantEntradasNecesarias = Entradas_necesarias(sentencia->Valor, (*instancia)->tamanio_Entrada);
    bool encontramosLaCantidadNecesaria = false;
    //bool elCandidatoEsNULL = candidato == NULL;

    //printf("Cont %d nec %d enc %d\n", contador, cantEntradasNecesarias, encontramosLaCantidadNecesaria);

    //if(cantEntradasNecesarias > list_filter(tablaEntradas, _es_libre)->elements_count) return NULL;

    while(!encontramosLaCantidadNecesaria && aux != NULL) {
        
        if(Es_entrada_libre((t_entrada *)aux->data)){
            //printf("Entramos true \n");
            aux = aux->next;
            contador++;
        }
        else {
            //printf("Entramos else\n");
            aux = aux->next;
            candidato = aux;
            contador = 0;
        }

        encontramosLaCantidadNecesaria = contador == cantEntradasNecesarias;
        //printf("Dentro de while enc %d\n", encontramosLaCantidadNecesaria);
    }
    
    //printf("Salimos del while?\n");
    
    return encontramosLaCantidadNecesaria ? candidato : NULL;
}
#include "../includes/instancia.h"

Instancia * crear_instancia() {

    Instancia * instancia = NULL;
    instancia = malloc(sizeof(Instancia));
    instancia->id = -1;
    instancia->socket = -1;
    instancia->cantidad_Entradas = 0;
    instancia->tamanio_Entrada = 0;
    instancia->memoria = NULL;
    instancia->tamanio_Memoria = 0;
    instancia->tabla_Entradas = list_create();
    instancia->montaje = NULL;
    instancia->intervalo = 0;
    return instancia;
}

void liberar_instancia(Instancia **instancia) {

    free((*instancia)->memoria);
    list_destroy_and_destroy_elements((*instancia)->tabla_Entradas, free);
    free(instancia);
}
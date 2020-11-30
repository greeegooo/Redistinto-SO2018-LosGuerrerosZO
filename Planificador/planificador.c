#include "includes/planificador.h"
#include "includes/consola.h"

void Send_Handshake(int socket)
{
    Send_Payload(socket, HANDSHAKE_PLAN, SIN_PAYLOAD, SIN_DATOS);
}

void Send_Payload_Transaccion(int socket, ASUNTO asunto, int datos)
{

    Payload *payload = malloc(sizeof(Header));
    Header *header_Payload = Crear_Header(asunto, SIN_PAYLOAD, ID_PLANIFICADOR);
    header_Payload->Identificador_Transaccion = datos;
    memcpy(payload, header_Payload, sizeof(Header));
    Destruir_Header(header_Payload);
    //printf("Pid Payload %d\n", *(payload + (sizeof(Header) - sizeof(int))));
    int largoMensaje = send(socket, payload, sizeof(Header), 0);

    if (largoMensaje <= 0)
    {
        Eliminar_Socket(socket);
    }

    if (socket == socket_Coordinador && largoMensaje > 0)
    {
        log_info(logger, "Enviado correctamente mensaje al coordinador");
    }
    else if (largoMensaje > 0)
    {
        log_info(logger, "Enviado correctamente mensaje al cliente %d", socket);
    }
    else if (socket == socket_Coordinador && largoMensaje <= 0)
    {
        log_error(logger, "Se desconecto el coordinador, detectado al enviarle un mensaje, finalizando el planificador");
        GOODBYE_WORLD();
    }
    else if (largoMensaje == 0)
    {
        socket_A_Buscar = socket;
        log_error(logger, "Se desconecto el pid: %d, detectado al enviarle un mensaje", ((Pid_Socket *)list_find(lista_Pid_Socket, Es_Socket_ESI_En_Ejecucion_Por_Socket_A_Buscar))->Pid);
        Finalizar_ESI(header_Payload->IdSolicitante);
        //log_info(logger, "Se desconecto el socket: %d, detectado al enviarle un mensaje", socket);
    }
    else
    {
        log_error(logger, "Ocurrio un error al enviar un mensaje al socket: %d", socket);
    }

    free(payload);
}

int Aceptar_Conexion()
{
    struct sockaddr_in direccionCliente;
    int tamanioDireccion = sizeof(struct sockaddr_in);
    int cliente = accept(servidor, (struct sockaddr *)&direccionCliente, (socklen_t *)&tamanioDireccion);
    if (cliente < 0)
    {
        log_error(logger, "No pude aceptar al cliente %d\n", cliente);
        return -1;
    }

    log_info(logger, "Recibí una conexión en %d\n", cliente);
    return cliente;
}

void Actualizar_Datos_ESI_Ejecucion()
{
    pid_A_Buscar = pid_Ejecucion;
    ESI_Plan *esi_Plan = (ESI_Plan *)list_find(lista_ESI_Planificacion, Se_Encuentra_ESI_Plan_Por_Pid_A_Buscar);
    switch (algoritmo_Planificacion)
    {
    case FIFO:
        break;
    default:
    {
        esi_Plan->Rafagas_Efectuadas += 1;
    }
    }
}

bool Se_Encuentra_ESI_Plan_Por_Pid_A_Buscar(void *esi_Plan)
{
    return ((ESI_Plan *)esi_Plan)->Pid == pid_A_Buscar;
}

Header *Recibir_Header(int socket)
{

    Header *header = malloc(sizeof(Header));
    int largo_mensaje = recv(socket, header, sizeof(Header), 0);
    printf("Asunto recibido: %d\n\n", header->Asunto);
    printf("Del solicitante: %d\n\n", header->IdSolicitante);
    if (largo_mensaje == 0)
    {
        Destruir_Header(header);
        if (socket == socket_Coordinador)
        {
            log_error(logger, "Se desconecto el Coordinador");
            GOODBYE_WORLD();
        }
        else
        {
            socket_A_Buscar = socket;
            log_error(logger, "Se desconecto el pid: %d, detectado al enviarle un mensaje", ((Pid_Socket *)list_find(lista_Pid_Socket, Es_Socket_ESI_En_Ejecucion_Por_Socket_A_Buscar))->Pid);
            Eliminar_Socket(socket);
            //log_error(logger, "Se desconecto el cliente %d", socket);
        }
        header = NULL;
    }
    if (largo_mensaje < 0)
    {
        if (socket == socket_Coordinador)
        {
            log_error(logger, "Ocurrio un eror al intentar recibir la cabecera del Coordinador, cerrando la ejecución");
            GOODBYE_WORLD();
        }
        socket_A_Buscar = socket;
        log_error(logger, "No se pudo recibir la cabecera del pid: %d", ((Pid_Socket *)list_find(lista_Pid_Socket, Es_Socket_ESI_En_Ejecucion_Por_Socket_A_Buscar))->Pid);
        //log_error(logger, "No se pudo recibir la cabecera del cliente %d", socket);
        Destruir_Header(header);
        Eliminar_Socket(socket);
        header = NULL;
    }
    printf("OK HEADER\n\n");
    return header;
}

void Liberar_Todos_Los_Datos(int pid)
{
    pid_a_borrar = pid;
    Pid_Key *pid_Key;
    while ((list_any_satisfy(lista_Pid_Recurso, Se_Encuentra_Pid_A_Borrar_Por_Pid_Key)))
    {
        pid_Key = (Pid_Key *)list_remove_by_condition(lista_Pid_Recurso, Se_Encuentra_Pid_A_Borrar_Por_Pid_Key);
        log_info(logger, "Se libero la clave %s, del ESI %d", pid_Key->Key, pid_Key->Pid);
        Destruir_Pid_Key(pid_Key);
    }

    while ((list_any_satisfy(lista_Pid_Bloqueados, Se_Encuentra_Pid_A_Borrar_Por_Pid_Key)))
    {
        list_remove_and_destroy_by_condition(lista_Pid_Bloqueados, Se_Encuentra_Pid_A_Borrar_Por_Pid_Key, Destruir_Pid_Key);
    }

    list_remove_and_destroy_by_condition(lista_ESI_Planificacion, Es_Pid_Plan_A_Borrar, free);
}

bool Es_Pid_Plan_A_Borrar(void *esi_Plan)
{
    return pid_a_borrar == ((ESI_Plan *)esi_Plan)->Pid;
}

bool Es_Pid_Socket_Buscado(void *pid_Socket)
{
    return pid_Socket_A_Buscar == ((Pid_Socket *)pid_Socket)->Pid;
}

void Pasar_ESI_Finalizado(int pid)
{
    pid_A_Buscar = pid;
    while (list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado))
    {
        list_remove_by_condition(lista_Pid_Nuevos, Es_Pid_Buscado);
    }
    while (list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado))
    {
        list_remove_by_condition(lista_Pid_Desbloqueados, Es_Pid_Buscado);
    }
    while (list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado))
    {
        list_remove_by_condition(lista_Pid_Listos, Es_Pid_Buscado);
    }
    while (list_any_satisfy(lista_Pid_Ejecucion, Es_Pid_Buscado))
    {
        list_remove_by_condition(lista_Pid_Ejecucion, Es_Pid_Buscado);
    }
    while (list_any_satisfy(lista_Pid_Socket, Es_Pid_Buscado))
    {
        list_remove_by_condition(lista_Pid_Socket, Es_Pid_Buscado);
    }

    if (!list_any_satisfy(lista_Pid_Finalizados, Es_Pid_Buscado)) {
    list_add(lista_Pid_Finalizados, (void *)pid);
    }
}

bool Es_Pid_Buscado(void *pid)
{
    return pid_A_Buscar == (int)pid;
}

void Planificar()
{
    if (activarPlanificacion)
    {
        switch (algoritmo_Planificacion)
        {
            case HRRN:
            {
                Planificar_HRRN();
                break;
            }
            case SJF_SD:
            {
                printf("4\n");
                Planificar_SJF_SD();
                break;
            }
            case SJF_CD:
            {
                Planificar_SJF_CD();
                break;
            }
            default:
            {
                Planificar_FIFO();
                break;
            }
        }
        com_Imprimir();
        list_clean(lista_Pid_Desbloqueados);
        list_clean(lista_Pid_Nuevos);
        list_clean(lista_Pid_Ejecucion);
        for (int index = 0; index < list_size(lista_Pid_Listos); index++) {
            pid_A_Buscar = (int) list_get(lista_Pid_Listos, index);
            if(list_any_satisfy(lista_Pid_Bloqueados, Es_Clave_Pid_Recurso_Por_Pid_A_Buscar)) {
                list_remove(lista_Pid_Listos, index);
            }
        }
        com_Imprimir();
    }
    printf("5\n");
}

void Planificar_HRRN()
{
    log_info(logger, "Planificado de manera HRRN en el instante %d", reloj);

    if (Es_Ejecucion_Valida())
    {
        list_add_in_index(lista_Pid_Listos, 0, (void *)pid_Ejecucion);
    }

    printf("CANDIDADoS %d\n", list_size(lista_Pid_Listos));
    list_add_all(lista_Pid_Listos, lista_Pid_Desbloqueados);
    list_add_all(lista_Pid_Listos, lista_Pid_Nuevos);
    list_sort(lista_Pid_Listos, Es_ESI_Plan_Mayor_Prioridad_Multiplicadad_Eventos);
    printf("CANDIDADoS Add&Sorted %d\n", list_size(lista_Pid_Listos));

}

bool Es_Clave_Pid_Recurso_Por_Pid_A_Buscar(void * pid_Key) {
    return pid_A_Buscar == ((Pid_Key *) pid_Key)->Pid;
}

void Planificar_SJF_SD()
{
    log_info(logger, "Planificado de manera SJF Sin Desalojo en el instante %d", reloj);
    int pid_Aux;
    int index;
    pid_A_Buscar = pid_Ejecucion;

    if (!list_any_satisfy(lista_Pid_Finalizados, Es_Pid_Buscado) && reloj > 0 && !list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado) && !list_any_satisfy(lista_Pid_Bloqueados, Es_Clave_Pid_Recurso_Por_Pid_A_Buscar))
    {
        printf("OK agregue primero a PID %d\n", pid_Ejecucion);
        list_add_in_index(lista_Pid_Listos, 0, (void *) pid_Ejecucion);
    }
    else {
        printf("La cague borre a PID %d\n", pid_Ejecucion);
        while(list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado)) {
            list_remove_by_condition(lista_Pid_Listos, Es_Pid_Buscado);
        }
    }
    
    t_list *lista_Aux_ESI_Todos_Los_Tipos = list_duplicate(lista_Pid_Desbloqueados);

    list_add_all(lista_Aux_ESI_Todos_Los_Tipos, list_duplicate(lista_Pid_Nuevos));

    for (index = 0; index < list_size(lista_Aux_ESI_Todos_Los_Tipos); index++)
    {
        printf("index %d\n\n", index);
        pid_Aux = (int)list_get(lista_Aux_ESI_Todos_Los_Tipos, index);
        list_add(lista_Pid_Listos, (void *)pid_Aux);
    }

    list_sort(lista_Pid_Listos, Es_ESI_Plan_Menor_Tiempo_Multiplicadad_Eventos);

    log_info(logger, "Cantidad de ESIS planificados %d", list_size(lista_Aux_ESI_Todos_Los_Tipos));

    list_destroy(lista_Aux_ESI_Todos_Los_Tipos);
}

void Planificar_SJF_CD()
{
    log_info(logger, "Planificado de manera SJF Con Desalojo en el instante %d", reloj);
    pid_A_Buscar = pid_Ejecucion;
    if (!list_is_empty(lista_Pid_Listos) && pid_Ejecucion == (int) list_get(lista_Pid_Listos, 0)) {
        while (pid_Ejecucion == (int) list_get(lista_Pid_Listos, 0)) {
            list_remove_first(lista_Pid_Listos);
        }
        if (Es_Ejecucion_Valida()) {
        list_add_in_index(lista_Pid_Listos, 0, (void *) pid_Ejecucion);
        }
    }

    printf("TAMANIO PLANIFICADO SJF CD %d\n", list_size(lista_Pid_Listos));

    list_add_all(lista_Pid_Listos, lista_Pid_Desbloqueados);
    printf("TAMANIO PLANIFICADO SJF CD %d\n", list_size(lista_Pid_Listos));

    list_add_all(lista_Pid_Listos, lista_Pid_Nuevos);
    printf("TAMANIO PLANIFICADO SJF CD %d\n", list_size(lista_Pid_Listos));

    list_sort(lista_Pid_Listos, Es_ESI_Plan_Menor_Tiempo_Multiplicadad_Eventos);
    printf("TAMANIO PLANIFICADO SJF CD %d\n", list_size(lista_Pid_Listos));
}

void Planificar_FIFO()
{
    log_info(logger, "Planificado de manera FIFO en el instante %d", reloj);
    int pid_Aux;
    if (!list_is_empty(lista_Pid_Ejecucion))
    {
        pid_Aux = (int)list_remove_first(lista_Pid_Ejecucion);
        list_add_in_index(lista_Pid_Listos, 0, (void *)pid_Aux);
    }

    t_list *lista_Aux_ESI_Todos_Los_Tipos = list_duplicate(lista_Pid_Desbloqueados);
    list_add_all(lista_Aux_ESI_Todos_Los_Tipos, list_duplicate(lista_Pid_Nuevos));
    list_sort(lista_Aux_ESI_Todos_Los_Tipos, Es_ESI_Plan_Creado_Mas_Antiguo_Multiplicidad_Eventos);

    list_add_all(lista_Pid_Listos, lista_Aux_ESI_Todos_Los_Tipos);
    printf("CANDIDADoS %d\n", list_size(lista_Pid_Listos));

    list_destroy(lista_Aux_ESI_Todos_Los_Tipos);
}

bool Es_ESI_Plan_Menor_Tiempo(void *esi_Plan1, void *esi_Plan2)
{
    return ((ESI_Plan *)esi_Plan1)->Estimacion_ESI < ((ESI_Plan *)esi_Plan2)->Estimacion_ESI;
}

bool Es_ESI_Plan_Creado_Mas_Antiguo(void *esi_Plan1, void *esi_Plan2)
{
    return ((ESI_Plan *)esi_Plan1)->Instante_Creacion <= ((ESI_Plan *)esi_Plan2)->Instante_Creacion;
}

bool Es_ESI_Plan_Desbloqueado_Mas_Antiguo(void *esi_Plan1, void *esi_Plan2)
{
    return ((ESI_Plan *)esi_Plan1)->Instante_Desbloqueo <= ((ESI_Plan *)esi_Plan2)->Instante_Desbloqueo;
}

bool Es_ESI_Plan_Creado_Mas_Antiguo_Multiplicidad_Eventos(void *pid1, void *pid2)
{
    pid_Planificacion = (int)pid1;
    pid_Planificacion_Aux = (int)pid2;
    ESI_Plan *esi_Plan1 = (ESI_Plan *)list_find(lista_ESI_Planificacion, Es_ESI_Planificacion_Por_Pid_Planificacion);
    ESI_Plan *esi_Plan2 = (ESI_Plan *)list_find(lista_ESI_Planificacion, Es_ESI_Planificacion_Por_Pid_Planificacion_Aux);
    bool passCondition;

    log_info(logger, "Datos de planificacion del ESI con PID: %d, Instante Creacion: %d, Instante Desbloqueo (considerar Cero nunca bloqueado): %d, Cantidad de ráfagas consumidas: %d, Estimacion actual: %f, Tiempo de espera desde estar Listo: %f",
             esi_Plan1->Pid, esi_Plan1->Instante_Creacion, esi_Plan1->Instante_Desbloqueo, esi_Plan1->Rafagas_Efectuadas, esi_Plan1->Estimacion_ESI, esi_Plan1->Tiempo_Espera);
    log_info(logger, "Datos de planificacion del ESI con PID: %d, Instante Creacion: %d, Instante Desbloqueo (considerar Cero nunca bloqueado): %d, Cantidad de ráfagas consumidas: %d, Estimacion actual: %f, Tiempo de espera desde estar Listo: %f",
             esi_Plan1->Pid, esi_Plan2->Instante_Creacion, esi_Plan2->Instante_Desbloqueo, esi_Plan2->Rafagas_Efectuadas, esi_Plan2->Estimacion_ESI, esi_Plan2->Tiempo_Espera);
    if (list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion) && list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion_Aux))
    {
        passCondition = esi_Plan1->Instante_Desbloqueo <= esi_Plan2->Instante_Desbloqueo;
    }
    else if (list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion) && list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado_Por_Pid_Planificacion_Aux))
    {
        passCondition = esi_Plan1->Instante_Desbloqueo <= esi_Plan2->Instante_Creacion;
    }
    else if (list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado_Por_Pid_Planificacion) && list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion_Aux))
    {
        passCondition = esi_Plan1->Instante_Creacion < esi_Plan2->Instante_Desbloqueo;
    }
    else
    {
        passCondition = esi_Plan1->Instante_Creacion <= esi_Plan2->Instante_Creacion;
    }

    return passCondition;
}

bool Es_ESI_Plan_Menor_Tiempo_Multiplicadad_Eventos(void *pid1, void *pid2)
{
    pid_Planificacion = (int)pid1;
    pid_Planificacion_Aux = (int)pid2;
    ESI_Plan *esi_Plan1 = (ESI_Plan *)list_find(lista_ESI_Planificacion, Es_ESI_Planificacion_Por_Pid_Planificacion);
    ESI_Plan *esi_Plan2 = (ESI_Plan *)list_find(lista_ESI_Planificacion, Es_ESI_Planificacion_Por_Pid_Planificacion_Aux);
    bool passCondition;

    log_info(logger, "Datos de planificacion del ESI con PID: %d, Instante Creacion: %d, Instante Desbloqueo (considerar Cero nunca bloqueado): %d, Cantidad de ráfagas consumidas: %d, Estimacion actual: %f, Tiempo de espera desde estar Listo: %f",
             esi_Plan1->Pid, esi_Plan1->Instante_Creacion, esi_Plan1->Instante_Desbloqueo, esi_Plan1->Rafagas_Efectuadas, esi_Plan1->Estimacion_ESI, esi_Plan1->Tiempo_Espera);
    log_info(logger, "Datos de planificacion del ESI con PID: %d, Instante Creacion: %d, Instante Desbloqueo (considerar Cero nunca bloqueado): %d, Cantidad de ráfagas consumidas: %d, Estimacion actual: %f, Tiempo de espera desde estar Listo: %f",
             esi_Plan1->Pid, esi_Plan2->Instante_Creacion, esi_Plan2->Instante_Desbloqueo, esi_Plan2->Rafagas_Efectuadas, esi_Plan2->Estimacion_ESI, esi_Plan2->Tiempo_Espera);
    if (esi_Plan1->Estimacion_ESI == esi_Plan2->Estimacion_ESI)
    {
        if (pid_Planificacion == pid_Ejecucion)
        {
            passCondition = true;
        }
        else if (pid_Planificacion_Aux == pid_Ejecucion)
        {
            passCondition = false;
        }
        else if (list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado_Por_Pid_Planificacion)) {
            passCondition = esi_Plan1->Instante_Creacion <= esi_Plan2->Instante_Creacion;
        }
        else if (list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado_Por_Pid_Planificacion_Aux)) {
            if (list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado_Por_Pid_Planificacion)) {
                passCondition = esi_Plan2->Instante_Creacion < esi_Plan1->Instante_Creacion;
            }
            else {
                passCondition = esi_Plan2->Instante_Creacion <= esi_Plan1->Instante_Creacion;
            }
        }
        else if (list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion) && list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion_Aux))
        {
            passCondition = esi_Plan1->Instante_Desbloqueo < esi_Plan2->Instante_Desbloqueo;
        }
        else if (list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado_Por_Pid_Planificacion) && list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado_Por_Pid_Planificacion_Aux))
        {
            passCondition = esi_Plan1->Instante_Creacion <= esi_Plan2->Instante_Creacion;
        }
        else if (list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion) && list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado_Por_Pid_Planificacion_Aux))
        {
            passCondition = esi_Plan1->Instante_Desbloqueo <= esi_Plan2->Instante_Creacion;
        }
        else
        {
            passCondition = esi_Plan1->Instante_Creacion < esi_Plan2->Instante_Desbloqueo;
        }
    }
    else
    {
        passCondition = esi_Plan1->Estimacion_ESI < esi_Plan2->Estimacion_ESI;
    }

    return passCondition;
}

bool Es_ESI_Plan_Mayor_Prioridad_Multiplicadad_Eventos(void *pid1, void *pid2)
{
    pid_Planificacion = (int)pid1;
    pid_Planificacion_Aux = (int)pid2;
    ESI_Plan *esi_Plan1 = (ESI_Plan *)list_find(lista_ESI_Planificacion, Es_ESI_Planificacion_Por_Pid_Planificacion);
    ESI_Plan *esi_Plan2 = (ESI_Plan *)list_find(lista_ESI_Planificacion, Es_ESI_Planificacion_Por_Pid_Planificacion_Aux);
    bool passCondition;

    Calcular_Ratio_Response_ESI(esi_Plan1);
    Calcular_Ratio_Response_ESI(esi_Plan2);

    if (esi_Plan1->Ratio_Response == esi_Plan2->Ratio_Response)
    {
        if (list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado_Por_Pid_Planificacion)) {
            passCondition = esi_Plan1->Instante_Creacion <= esi_Plan2->Instante_Creacion;
        }
        else if (list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado_Por_Pid_Planificacion_Aux)) {
            if (list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado_Por_Pid_Planificacion)) {
                passCondition = esi_Plan2->Instante_Creacion < esi_Plan1->Instante_Creacion;
            }
            else {
                passCondition = esi_Plan2->Instante_Creacion <= esi_Plan1->Instante_Creacion;
            }
        }
        if (list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion) && list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion_Aux))
        {
            passCondition = esi_Plan1->Instante_Desbloqueo <= esi_Plan2->Instante_Desbloqueo;
        }
        else if (list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado_Por_Pid_Planificacion) && list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado_Por_Pid_Planificacion_Aux))
        {
            passCondition = esi_Plan1->Instante_Creacion <= esi_Plan2->Instante_Creacion;
        }
        else if (list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado_Por_Pid_Planificacion) && list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado_Por_Pid_Planificacion_Aux))
        {
            passCondition = esi_Plan1->Instante_Desbloqueo <= esi_Plan2->Instante_Creacion;
        }
        else
        {
            passCondition = esi_Plan1->Instante_Creacion < esi_Plan2->Instante_Desbloqueo;
        }
    }
    else
    {
        passCondition = esi_Plan1->Ratio_Response > esi_Plan2->Ratio_Response;
    }

    return passCondition;
}

void Calcular_Ratio_Response_ESI(ESI_Plan *esi_Plan)
{
    esi_Plan->Ratio_Response = (esi_Plan->Estimacion_ESI + esi_Plan->Tiempo_Espera) / esi_Plan->Estimacion_ESI;
}

void Actualizar_Reloj()
{
    pthread_mutex_unlock(&sem_ejecucion);
    reloj++;
    if (algoritmo_Planificacion == HRRN)
    {
        Actualizar_Tiempos_Espera_ESI();
    }
}

bool Es_ESI_Planificacion_Por_Pid_Planificacion(void *esi_Plan)
{
    return pid_Planificacion == ((ESI_Plan *)esi_Plan)->Pid;
}

bool Es_ESI_Planificacion_Por_Pid_Planificacion_Aux(void *esi_Plan)
{
    return pid_Planificacion_Aux == ((ESI_Plan *)esi_Plan)->Pid;
}

bool Es_Pid_Buscado_Por_Pid_Planificacion(void *pid)
{
    return pid_Planificacion == (int)pid;
}

bool Es_Pid_Buscado_Por_Pid_Planificacion_Aux(void *pid)
{
    return pid_Planificacion_Aux == (int)pid;
}

void Actualizar_Pid()
{
    ultimo_Pid++;
}

int Proceso_Nuevo(int socket)
{
    ESI_Plan *esi_plan = malloc(sizeof(ESI_Plan));
    esi_plan->Pid = ultimo_Pid;
    esi_plan->Instante_Creacion = reloj;
    esi_plan->Instante_Desbloqueo = 0;
    esi_plan->Tiempo_Espera = 0.0;
    esi_plan->Rafagas_Efectuadas = 0;
    esi_plan->Estimacion_ESI = estimacion_Inicial_ESI;
    esi_plan->Ratio_Response = 1.0;
    Actualizar_Pid();
    list_add(lista_ESI_Planificacion, (void *)esi_plan);

    Pid_Socket *pid_socket = malloc(sizeof(Pid_Socket));
    pid_socket->Socket = socket;
    pid_socket->Pid = esi_plan->Pid;
    list_add(lista_Pid_Socket, (void *)pid_socket);
    list_add_in_index(lista_Pid_Nuevos, list_size(lista_Pid_Nuevos),(void *)esi_plan->Pid);

    return esi_plan->Pid;
}

bool Es_Socket_Pid_Ejecucion(void *pid)
{
    return pid_Ejecucion == *(int *)pid;
}

bool Es_Socket_Pid_Por_Pid_Ejecucion(void * socket_Pid)
{
    return pid_Ejecucion == ((Pid_Socket *)socket_Pid)->Pid;
}


void Asignar_Pid_Key(int pid, char *clave)
{
    Pid_Key *pid_Key = malloc(sizeof(Pid_Key));
    pid_Key->Pid = pid;
    pid_Key->Key = clave;
    list_add(lista_Pid_Recurso, (void *)pid_Key);
}

int Encontrar_Socket_Pid_Ejecucion()
{
    printf("SOS UN COMUÑE FABULOSOOOO \n\n");
    Pid_Socket * pid_Socket_AUX;
    for (int index = 0; index < list_size(lista_Pid_Socket); index++) {
        pid_Socket_AUX = (Pid_Socket *) list_get(lista_Pid_Socket, index);
        printf("El Socket que encontré bien manopla puto PID %d para el próximo a ejecutar es: %d\n\n", pid_Socket_AUX->Pid, pid_Socket_AUX->Socket);   
    }
    Pid_Socket *pid_Socket = (Pid_Socket *)list_find(lista_Pid_Socket, Es_Socket_Pid_Por_Pid_Ejecucion);
    if (pid_Socket == NULL) {
        return 2;
    }
    else {
    return pid_Socket->Socket;
    }
    printf("El Socket que encontré para el próximo a ejecutar es: %d\n\n", pid_Socket->Socket);
}

void Ejecutar_ESI_Listo()
{
    if (!list_is_empty(lista_Pid_Listos) && activarPlanificacion)
    {
        enEjecucion = true;
        Pid_Key *pid_Key;
        for (int index = 0; index < list_size(lista_Pid_Socket); index++)
        {
            pid_Key = (Pid_Key *)list_get(lista_Pid_Socket, index);
            log_error(logger, "Los socket pid son Pid:%d, con cliente %d", pid_Key->Pid, pid_Key->Key);
        }

        printf("POR ACA SIIIIIIIi EJECUCION DE UN NUEVO COMUÑE\n");

        ESI_Plan *esi_Plan;
        pid_Ejecucion = (int)list_remove_first(lista_Pid_Listos);
        pid_A_Buscar = pid_Ejecucion;
        printf("PID A EJECUTAR: %d\n", pid_Ejecucion);
        Imprimir_Lista_ESI_PLAN();
        esi_Plan = (ESI_Plan *)list_find(lista_ESI_Planificacion, ES_ESI_Plan_Buscado_Por_Pid_A_Buscar);
        printf ("OKKKAAASAA\n");
        esi_Plan->Tiempo_Espera = 0.0;
        printf ("OKKKAAASAA2323232\n");
        list_add(lista_Pid_Ejecucion, (void *)pid_Ejecucion);
        int socket = Encontrar_Socket_Pid_Ejecucion();
        printf("OLA K ASEqweqw\n");
        if (algoritmo_Planificacion != FIFO)
        {
            log_info(logger, "La estimación del ESI %d es: %f", pid_Ejecucion, esi_Plan->Estimacion_ESI);
        }
        if (algoritmo_Planificacion == HRRN)
        {
            log_info(logger, "El Ratio Response es: %f", esi_Plan->Ratio_Response);
        }
        printf("OLA K ASE\n");
        
        Send_Payload(socket, EJECUTATE, SIN_PAYLOAD, SIN_DATOS);
        log_info(logger, "Mandado Asunto EJECUTATE al ESI con PID: %d, en el socket: %d, en el instante: %d", pid_Ejecucion, socket, reloj);

        ultimo_Reloj_Ejecucion = reloj;
    }
    else if (list_is_empty(lista_Pid_Listos))
    {
        enEjecucion = false;
        log_info(logger, "No hay ESIS listos para ejecutar");
    }
    else {
        log_info(logger, "La planificación se encuentra pausada");
    }
}

void Mandar_Ejecutate_ESI_Ejecucion()
{
    if (activarPlanificacion) {
        enEjecucion = true;
        Pid_Key *pid_Key;
        for (int index = 0; index < list_size(lista_Pid_Socket); index++)
        {
            pid_Key = (Pid_Key *)list_get(lista_Pid_Socket, index);
            log_error(logger, "Los socket pid son Pid:%d, con cliente %d", pid_Key->Pid, pid_Key->Key);
        }
        printf("BOLUUUUDOOOOOO POR ACA NOOOOO \n");
        ESI_Plan *esi_Plan;
        int socket = Encontrar_Socket_Pid_Ejecucion();
        pid_A_Buscar = pid_Ejecucion;
        esi_Plan = (ESI_Plan *)list_find(lista_ESI_Planificacion, ES_ESI_Plan_Buscado_Por_Pid_A_Buscar);
        esi_Plan->Tiempo_Espera = 0.0;
        if (algoritmo_Planificacion != FIFO)
        {
            log_info(logger, "La estimación del ESI %d es: %f", pid_Ejecucion, esi_Plan->Estimacion_ESI);
        }
        if (algoritmo_Planificacion == HRRN)
        {
            log_info(logger, "El Ratio Response es: %f", esi_Plan->Ratio_Response);
        }
        Send_Payload(socket, EJECUTATE, SIN_PAYLOAD, SIN_DATOS);
        log_info(logger, "Mandado Asunto EJECUTATE al ESI con PID: %d, en el socket: %d en el instante: %d", pid_Ejecucion, socket, reloj);
        ultimo_Reloj_Ejecucion = reloj;
    }
    else {
        log_info(logger, "La planificación se encuentra pausada, no se puede continuar ejecutando el ESI %d", pid_Ejecucion);
    }
}

void Cargar_Algoritmo(char *string_algoritmo_Planificacion)
{
    if (strcmp(string_algoritmo_Planificacion, "HRRN") == 0)
    {
        algoritmo_Planificacion = HRRN;
    }
    else if (strcmp(string_algoritmo_Planificacion, "SJF_SD") == 0)
    {
        algoritmo_Planificacion = SJF_SD;
    }
    else if (strcmp(string_algoritmo_Planificacion, "SJF_CD") == 0)
    {
        algoritmo_Planificacion = SJF_CD;
    }
    else
    {
        algoritmo_Planificacion = FIFO;
    }

    free(string_algoritmo_Planificacion);
}

void Actualizar_Tiempos_Espera_ESI()
{
    int index;
    ESI_Plan *esi_Plan;
    for (index = 0; index < list_size(lista_Pid_Listos); index++)
    {
        pid_A_Buscar = (int)list_get(lista_Pid_Listos, index);
        esi_Plan = (ESI_Plan *)list_find(lista_ESI_Planificacion, ES_ESI_Plan_Buscado_Por_Pid_A_Buscar);
        esi_Plan->Tiempo_Espera += 1.0;
        Calcular_Ratio_Response_ESI(esi_Plan);
    }
    for (index = 0; index < list_size(lista_Pid_Nuevos); index++)
    {
        pid_A_Buscar = (int)list_get(lista_Pid_Nuevos, index);
        esi_Plan = (ESI_Plan *)list_find(lista_ESI_Planificacion, ES_ESI_Plan_Buscado_Por_Pid_A_Buscar);
        esi_Plan->Tiempo_Espera += 1.0;
        Calcular_Ratio_Response_ESI(esi_Plan);
    }
    for (index = 0; index < list_size(lista_Pid_Desbloqueados); index++)
    {
        pid_A_Buscar = (int)list_get(lista_Pid_Desbloqueados, index);
        esi_Plan = (ESI_Plan *)list_find(lista_ESI_Planificacion, ES_ESI_Plan_Buscado_Por_Pid_A_Buscar);
        Calcular_Ratio_Response_ESI(esi_Plan);
    }
}

bool ES_ESI_Plan_Buscado_Por_Pid_A_Buscar(void *esi_Plan)
{
    printf("PID EJECUCION %d, PID ESI PLAN %d\n", pid_Ejecucion, ((ESI_Plan *) esi_Plan)->Pid);
    return pid_A_Buscar == ((ESI_Plan *)esi_Plan)->Pid;
}

void Abortar_ESI_Ejecucion(bool comunicar)
{
    int socket = Encontrar_Socket_Pid_Ejecucion();
    if (comunicar) {
    Send_Payload(socket, ABORTAR_ESI, SIN_PAYLOAD, SIN_DATOS);
    }
    Eliminar_Socket_Ejecucion(socket);
}

void Finalizar_ESI(int pid)
{
    Liberar_Todos_Los_Datos(pid);
    Pasar_ESI_Finalizado(pid);
}

void Liberar_Clave(char *clave)
{
    strcpy(claveADesbloquear, clave);
    Pid_Key *pid_Key = (Pid_Key *)list_remove_by_condition(lista_Pid_Recurso, Clave_A_Desbloquear);
    Liberar_ESIS_Bloqueados_Por_Clave(clave);
    log_info(logger, "Se libero la clave %s del ESI %d", pid_Key->Key, pid_Key->Pid);
    Destruir_Pid_Key(pid_Key);
}

void Liberar_ESIS_Bloqueados_Por_Clave(char *clave)
{
    Pid_Key *pid_Key_Aux;
    strcpy(claveADesbloquear, clave);
    while (list_any_satisfy(lista_Pid_Bloqueados, Clave_A_Desbloquear))
    {
        pid_Key_Aux = (Pid_Key *)list_find(lista_Pid_Bloqueados, Clave_A_Desbloquear);
        list_add_in_index(lista_Pid_Desbloqueados, list_size(lista_Pid_Desbloqueados), (void *)pid_Key_Aux->Pid);
        log_info(logger, "Se desbloqueo el ESI con PID: %d", pid_Key_Aux->Pid);
        Destruir_Pid_Key(pid_Key_Aux);
    }
}

bool Clave_Disponible(char *clave, int pid)
{
    strcpy(claveADesbloquear, clave);
    Pid_Key * pid_Key = list_find(lista_Pid_Recurso, Clave_A_Desbloquear);
    printf("VAMOOOSO No te Duermas buuuuachin \n");
    return pid_Key == NULL || (pid_Key != NULL && ((Pid_Key *)pid_Key)->Pid == pid);
    printf("VAMOOOSO GUACHIIINNN \n");
}

void Bloquear_ESI_Ejecucion(char *clave)
{
    Pid_Key *pid_Key = malloc(sizeof(Pid_Key));
    pid_Key->Pid = pid_Ejecucion;
    pid_Key->Key = clave;
    Estimar_Tiempo_ESI(pid_Ejecucion);
    list_add(lista_Pid_Bloqueados, (void *)pid_Key);
    Eliminar_De_Listas_Menos_Bloqueado(pid_Ejecucion);
    log_info(logger, "Se bloqueo el ESI %d con la clave %s", pid_Ejecucion, clave);
}

void Eliminar_De_Listas_Menos_Bloqueado(int pid) {
    pid_A_Buscar = pid;
    while(list_any_satisfy(lista_Pid_Listos, Es_Pid_Buscado)) {
        list_remove_by_condition(lista_Pid_Listos, Es_Pid_Buscado);
    }
    while(list_any_satisfy(lista_Pid_Nuevos, Es_Pid_Buscado)) {
        list_remove_by_condition(lista_Pid_Listos, Es_Pid_Buscado);
    }
    while(list_any_satisfy(lista_Pid_Desbloqueados, Es_Pid_Buscado)) {
        list_remove_by_condition(lista_Pid_Listos, Es_Pid_Buscado);
    }
}

void Estimar_Tiempo_ESI(int pid)
{
    ultimo_Reloj_ESI_Estimado = reloj;
    pid_A_Buscar = pid;
    if (list_any_satisfy(lista_ESI_Planificacion, ES_ESI_Plan_Buscado_Por_Pid_A_Buscar))
    {
        ESI_Plan *esi_Plan = (ESI_Plan *)list_find(lista_ESI_Planificacion, ES_ESI_Plan_Buscado_Por_Pid_A_Buscar);
        esi_Plan->Estimacion_ESI = (esi_Plan->Rafagas_Efectuadas * alfa_Planificacion) + ((1 - alfa_Planificacion) * esi_Plan->Estimacion_ESI);
    }
    else {
        printf("NO TE AUMENTO LA ESTIMACION EN NADA TROLIN\n");
    }
}

void *list_remove_first(t_list *lista)
{
    return list_remove(lista, 0);
}

void Send_Payload(int socket, ASUNTO asunto, int tamanioDatos, void *datos)
{
    log_info(logger, "Enviado Payload a cliente %d, con ASUNTO: %d, tamaño: %d", socket, asunto, tamanioDatos);
    printf("OLAKEASAS\n");
    Payload *payload = malloc(sizeof(Header) + tamanioDatos);
    Header *header_Payload = Crear_Header(asunto, tamanioDatos, ID_PLANIFICADOR);
    memcpy(payload, header_Payload, sizeof(Header));
    Destruir_Header(header_Payload);
    if (tamanioDatos > 0)
    {
        memcpy(payload + sizeof(Header), &datos, tamanioDatos);
    }
    int largo_mensaje = send(socket, payload, sizeof(Header) + tamanioDatos, 0);
    if (socket == socket_Coordinador && largo_mensaje > 0)
    {
        log_info(logger, "Enviado correctamente mensaje al coordinador");
    }
    else if (socket == socket_Coordinador && largo_mensaje <= 0)
    {
        log_error(logger, "Ocurrió un error al intentar enviar un mensaje al coordinador, el mismo ya no está presente, abortando la ejecución", socket);
        GOODBYE_WORLD();
    }
    else if (socket != socket_Coordinador && largo_mensaje <= 0)
    {
        log_error(logger, "Ocurrió un error al intentar enviar un mensaje al cliente %d", socket);
        Eliminar_Socket(socket);
    }
    else
    {
        log_info(logger, "Enviado correctamente mensaje al cliente %d", socket);
    }
    free(payload);
    free(datos);
}

void *Recibir_Datos(int socket, int tamanioDatos)
{
    void *data = malloc(tamanioDatos);
    int largo_mensaje = recv(socket, data, tamanioDatos, 0);
    if (largo_mensaje == 0)
    {
        free(data);
        data = NULL;
        if (socket == socket_Coordinador)
        {
            log_error(logger, "Se desconecto el Coordinador al intentar recibir datos, terminando la ejecución");
            GOODBYE_WORLD();
        }
        else
        {
            Eliminar_Socket(socket);
            socket_A_Buscar = socket;
            log_error(logger, "Se desconecto el pid: %d, detectado al enviarle un mensaje", ((Pid_Socket *)list_find(lista_Pid_Socket, Es_Socket_ESI_En_Ejecucion_Por_Socket_A_Buscar))->Pid);
            //log_error(logger, "Se desconecto el cliente %d", socket);
        }
    }
    else if (largo_mensaje < 0)
    {
        if (socket == socket_Coordinador)
        {
            log_info(logger, "No se pudo recibir los datos del coordinador, terminando la ejecución");
            GOODBYE_WORLD();
        }
        log_error(logger, "No se pudo recibir los datos del cliente %d", socket);
        free(data);
        Eliminar_Socket(socket);
        data = NULL;
    }
    return data;
}

void Eliminar_Socket(int socket)
{
    close(socket);
    FD_CLR(socket, &master);
    if (socket != socket_Coordinador)
    {
        pid_Socket_A_Buscar = socket;
        Pid_Socket *pid_Socket = (Pid_Socket *)list_remove_by_condition(lista_Pid_Socket, Es_Socket_ESI_En_Ejecucion_Por_Socket_A_Buscar);
        Finalizar_ESI(pid_Socket->Pid);
        if (pid_Socket != NULL && pid_Socket->Pid == pid_Ejecucion)
        {
            log_info(logger, "Se cerro la comunicación debido a un error con el ESI con PID: %d y Socket: %d, se replanifica en el instante %d", pid_Socket->Pid, pid_Socket->Socket, reloj);
            enEjecucion = false;
            Planificar();
            Ejecutar_ESI_Listo();
        }
        if (pid_Socket != NULL) {
        free(pid_Socket);
        }
    }
}

void Eliminar_Socket_Ejecucion(int socket)
{
    close(socket);
    FD_CLR(socket, &master);
    if (socket != socket_Coordinador)
    {
        enEjecucion = false;
        pid_Socket_A_Buscar = socket;
        Pid_Socket *pid_Socket = (Pid_Socket *)list_remove_by_condition(lista_Pid_Socket, Es_Socket_ESI_En_Ejecucion_Por_Socket_A_Buscar);
        log_info(logger, "Finalizo el ESI con PID %d del socket %d", pid_Socket->Pid, pid_Socket->Socket);
        Finalizar_ESI(pid_Socket->Pid);
        free(pid_Socket);
    }
}

bool Es_Socket_ESI_En_Ejecucion_Por_Socket_A_Buscar(void *pid_Socket)
{
    return pid_Socket_A_Buscar == ((Pid_Socket *)pid_Socket)->Socket;
}

bool Es_Socket_Pid_Socket_Por_Socket_A_Buscar(void *pid_Socket)
{
    return socket_A_Buscar == ((Pid_Socket *)pid_Socket)->Socket;
}

void GOODBYE_WORLD()
{
    exit(EXIT_SUCCESS);
}

bool Validar_Posesion_Key(int pid, Sentencia *sentencia)
{
    bool resultado;
    strcpy(claveRecurso, sentencia->Clave);
    Pid_Key * pid_Key = (Pid_Key *) list_find(lista_Pid_Recurso, Es_Clave_Pid_Recurso_Por_Clave_Recurso);
    if (pid_Key != NULL) {
        resultado = pid_Key->Pid == pid;
    }
    else {
        resultado = false;
    }
    if (resultado) {
        log_info(logger, "La clave %s pertenece al PID %d", claveRecurso, pid);
    }
    else {
        log_error(logger, "La clave %s no pertenece al PID %d", claveRecurso, pid);
    }
    return resultado;
}

bool Es_Clave_Pid_Recurso_Por_Clave_Recurso (void * pid_Key) {
    return strcmp(claveRecurso, ((Pid_Key *) pid_Key)->Key) == 0;
}

void Imprimir_Lista_ESI_PLAN()
{
    log_error(logger, "MOSTRANDO LISTA ESI PLAN");
    int index;
    ESI_Plan *esi_Plan;
    log_info(logger, "////////////////////////");
    for (index = 0; index < list_size(lista_ESI_Planificacion); index++)
    {
        esi_Plan = (ESI_Plan *)list_get(lista_ESI_Planificacion, index);
        log_info(logger, "PID: %d, Instante Creacion: %d, Estimacion_ESI: %d", esi_Plan->Pid, esi_Plan->Instante_Creacion, esi_Plan->Estimacion_ESI);
        log_info(logger, "Rafagas efectuadas: %d, Tiempo Desbloqueo: %d, Estimacion_ESI: %d", esi_Plan->Rafagas_Efectuadas, esi_Plan->Instante_Desbloqueo, esi_Plan->Ratio_Response);
        log_info(logger, "////////////////////////");
    }
    log_error(logger, "FIN LISTA ESI PLAN");
}

int main()
{

    //Se crea el logger.
    logger = log_create("planificador.log", "planificador", true, LOG_LEVEL_INFO);

    //Levantamos archivo de configuración.
    t_config *config = config_create("config.txt");

    if (config != NULL && !dictionary_is_empty(config->properties))
    {
        log_info(logger, "Las configuraciones fueron cargadas satisfactoriamente.\n\n");
    }
    else
    {
        log_error(logger, "Las configuraciones no fueron cargadas.\n\n");
    }

    int puerto_Escucha = config_get_int_value(config, "Puerto_Escucha");
    char *string_algoritmo_Planificacion = config_get_string_value(config, "Algoritmo_Planificacion");
    Cargar_Algoritmo(string_algoritmo_Planificacion);
    alfa_Planificacion = config_get_double_value(config, "Alfa_Planificacion");
    alfa_Planificacion = alfa_Planificacion / 100;
    estimacion_Inicial_ESI = (float)config_get_double_value(config, "Estimacion_Inicial");
    char *ip_Coordinador = config_get_string_value(config, "IP_Coordinador");
    char *puerto_Coordinador = config_get_string_value(config, "Puerto_Coordinador");
    char **claves_Bloqueadas = config_get_array_value(config, "Claves_Bloqueadas");

    pthread_mutex_init(&sem_ejecucion, NULL);
    //config_destroy(config);

    //Se crean las estructuras administrativas del Planificador.
    lista_Pid_Socket = list_create();
    lista_Pid_Recurso = list_create();
    lista_ESI_Planificacion = list_create();
    lista_Pid_Listos = list_create();
    lista_Pid_Nuevos = list_create();
    lista_Pid_Bloqueados = list_create();
    lista_Pid_Desbloqueados = list_create();
    lista_Pid_Finalizados = list_create();
    lista_Pid_Ejecucion = list_create();

    Inicializar_Lista_Pid_Recurso(lista_Pid_Recurso, claves_Bloqueadas);

    //Luego de las pruebas borrar
    printf("Se inicializó la lista de Claves bloqueadas:\n");
    for (int i = 0; i < list_size(lista_Pid_Recurso); i++)
    {
        printf("- %s\n", ((Pid_Key *)list_get(lista_Pid_Recurso, i))->Key);
    }
    printf("\n");

    //Se levanta la consola.
    pthread_t hiloConsola;
    pthread_create(&hiloConsola, NULL, &consolaPlan, NULL);
    log_info(logger, "La consola fue cargada correctamente.\n\n");

    // Levantamos el servidor.
    int fdmax;
    int clienteEscuchado;
    int escucharSocket;
    struct sockaddr_storage remoteaddr;
    socklen_t addrlen;

    struct sockaddr_in direccionServidor;
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_addr.s_addr = INADDR_ANY;
    direccionServidor.sin_port = htons(puerto_Escucha);
    servidor = socket(AF_INET, SOCK_STREAM, 0);

    int activado = 1;
    setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

    if (bind(servidor, (void *)&direccionServidor, sizeof(direccionServidor)) != 0)
    {
        log_error(logger, "Falló el bind.\n");
        return 1;
    }

    //Handshake con Coordinador
    socket_Coordinador = connect_to_server(logger, ip_Coordinador, puerto_Coordinador);
    log_info(logger, "Conectado al Coordinador");

    Header *header_Handshake;
    while (coordinadorInactivo)
    {

        header_Handshake = Recibir_Header(socket_Coordinador);

        if (header_Handshake->Asunto == HANDSHAKE_COORD)
        {
            Header * headerCoor = Crear_Header(HANDSHAKE_PLAN_COORD, SIN_PAYLOAD, ID_PLANIFICADOR);
            write(socket_Coordinador, headerCoor, sizeof(Header));
            free(headerCoor);
            log_info(logger, "El coordinador respondió mi saludo");
            coordinadorInactivo = false;
        }
        else
        {
            log_error(logger, "El coordinador me envió un asunto distinto al esperado, 'HANDSHAKE_COORD', envió %d", header_Handshake->Asunto);
        }
        free(header_Handshake);
    }

    //Iniciamos la escucha.
    log_info(logger, "Estoy escuchando.\n");
    int resListen = listen(servidor, 100);
    if (resListen < 0)
    {
        log_error(logger, "No estoy escuchando.\n");
        return 1;
    }

    FD_SET(servidor, &master);
    FD_SET(socket_Coordinador, &master);
    fdmax = servidor;
    Header *header;

    while (1)
    {
        read_fds = master; // copy it
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            log_error(logger, "Falló la multiplexacion");
            exit(FALLO_MULTIPLEXACION);
        }
    
        for(escucharSocket = 0; escucharSocket <= fdmax; escucharSocket++) {
            if (FD_ISSET(escucharSocket, &read_fds)) { // we got one!!
                if (escucharSocket == servidor) { // handle new connections
                addrlen = sizeof remoteaddr;
                clienteEscuchado = accept(servidor, (struct sockaddr *)&remoteaddr, &addrlen);
                    if (clienteEscuchado == -1) {
                        log_error(logger, "Ocurrio un error al intentar aceptar al cliente");
                    }
                    else
                    {
                        FD_SET(clienteEscuchado, &master); // add to master set
                        if (clienteEscuchado > fdmax)
                        { // keep track of the max
                            fdmax = clienteEscuchado;
                        }
                        log_info(logger, "Cliente nuevo en %d", clienteEscuchado);
                        Send_Handshake(clienteEscuchado);
                    }
                }
                else
                {
                    printf("socket escuchado: %d\n\n", escucharSocket);
                    header = Recibir_Header(escucharSocket);
                    if (header != NULL)
                    {
                        printf("ASUNTO header: %d\n Length: %d\n Id: %d\n\n", header->Asunto, header->Length, header->IdSolicitante);

                        switch (header->Asunto)
                        {
                        case NUEVO_ESI:
                        { 
                            int pid = (int)Proceso_Nuevo(escucharSocket);
                            printf("PID NUEVO: %d\n\n", pid);
                            Send_Payload_Transaccion(escucharSocket, ASIGNACION_PID, pid);
                            if (list_is_empty(lista_Pid_Listos)|| algoritmo_Planificacion == FIFO || algoritmo_Planificacion == SJF_CD) {
                                printf("3\n");
                                Planificar();
                            }
                            log_info(logger, "Reloj %d, Ultimo Reloj Estimado %d", reloj, ultimo_Reloj_ESI_Estimado);
                            pid_A_Buscar = pid_Ejecucion;
                            if (reloj > 0 && reloj > ultimo_Reloj_ESI_Estimado && algoritmo_Planificacion == SJF_CD && !list_any_satisfy(lista_Pid_Finalizados, Es_Pid_Buscado)) {
                                Estimar_Tiempo_ESI(pid_Ejecucion);
                            }

                            if (!enEjecucion) {
                                Ejecutar_ESI_Listo();
                            }
                            else {
                                Mandar_Ejecutate_ESI_Ejecucion();
                            }
                            break;
                        }
                        case SENTENCIA_EJECUTADA_CON_EXITO:
                        {
                            //pthread_mutex_unlock(&sem_ejecucion);
                            enEjecucion = false;
                            Actualizar_Reloj();
                            if (!list_is_empty(lista_Pid_Bloqueados) || !list_is_empty(lista_Pid_Nuevos))
                            {
                                Estimar_Tiempo_ESI(pid_Ejecucion);
                            }

                            if (algoritmo_Planificacion == SJF_CD && (!list_is_empty(lista_Pid_Bloqueados) || !list_is_empty(lista_Pid_Nuevos))) {
                            Planificar();
                            }

                            if(pid_Ejecucion != (int) list_get(lista_Pid_Listos, 0)) {
                                Ejecutar_ESI_Listo();
                            }
                            else {
                                Mandar_Ejecutate_ESI_Ejecucion();
                            }

                            break;
                        }
                        case SENTENCIA_EJECUTADA_CON_ERROR:
                        {
                            //pthread_mutex_unlock(&sem_ejecucion);
                            Abortar_ESI_Ejecucion(true);
                            Planificar();
                            Ejecutar_ESI_Listo();
                            break;
                        }
                        case ESI_PLAN_TERMINE:
                        {
                            printf("TERMINE %d\n", header->IdSolicitante);
                            Actualizar_Reloj();
                            Eliminar_Socket_Ejecucion(escucharSocket);
                            Imprimir_Lista_ESI_PLAN();
                            enEjecucion = false;
                            Planificar();
                            Ejecutar_ESI_Listo();
                            break;
                        }
                        case ESI_PLAN_ABORTE:
                        {
                            log_info(logger, "Aborto el ESI PID %d con socket %d", header->IdSolicitante, escucharSocket);
                            Liberar_Todos_Los_Datos(header->IdSolicitante);
                            Eliminar_Socket(escucharSocket);
                            //pthread_mutex_unlock(&sem_ejecucion);
                            enEjecucion = false;
                            Planificar();
                            Ejecutar_ESI_Listo();
                            break;
                        }
                        case SENTENCIA_PLANIFICADOR_REQ:
                        {
                            Sentencia *sentencia = Recibir_Sentencia(escucharSocket);
                            printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
                            switch (sentencia->Operacion)
                            {
                            case S_GET:
                            {
                                char *clave = malloc(sizeof(char) * 40);
                                strcpy(clave, sentencia->Clave);
                                bool disponible = Clave_Disponible(clave, header->IdSolicitante);
                                if (!disponible)
                                {
                                    log_info(logger, "Bloqueando ESI con PID %d por pedir la clave no disponible %s", header->IdSolicitante, sentencia->Clave);
                                    Bloquear_ESI_Ejecucion(clave);
                                    enEjecucion = false;    
                                    Planificar();
                                    Ejecutar_ESI_Listo();
                                }
                                else
                                {
                                    Asignar_Pid_Key(header->IdSolicitante, clave);
                                }
                                //Enviar_Sentencia(escucharSocket, sentencia, disponible ? OK : BLOQUEAR, ID_PLANIFICADOR);
                                sentencia->Resultado = disponible ? OK : BLOQUEAR;
                                Enviar_Sentencia(escucharSocket, sentencia, SENTENCIA_PLANIFICADOR_RESP, header->IdSolicitante);
                            }
                            break;
                            case S_SET:
                            {
                                //sentencia->Resultado = OK;

                                if (Validar_Posesion_Key(header->IdSolicitante, sentencia))
                                {
                                    sentencia->Resultado = OK;
                                    Enviar_Sentencia(escucharSocket, sentencia, SENTENCIA_PLANIFICADOR_RESP, header->IdSolicitante);
                                }
                                else
                                {
                                    sentencia->Resultado = ABORTAR;
                                    Enviar_Sentencia(escucharSocket, sentencia, SENTENCIA_PLANIFICADOR_RESP, header->IdSolicitante);
                                    //com_Kill(header->IdSolicitante);
                                    Abortar_ESI_Ejecucion(false);
                                    //int socket_ESI = ((Pid_Socket * ) list_find(lista_Pid_Socket, Es_Socket_Pid_Ejecucion))->Socket;
                                    //Eliminar_Socket_Ejecucion(socket_ESI);
                                    Planificar();
                                    Ejecutar_ESI_Listo();
                                    log_error(logger, "El ESI %d fue abortado\n", header->IdSolicitante);
                                    printf("PASE POR ACACACACAA S_SET\n");
                                }
                                printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
                            }
                            break;
                            case S_STORE:
                            {
                                char *clave = malloc(sizeof(char) * 40);
                                strcpy(clave, sentencia->Clave);

                                //Enviar_Sentencia(escucharSocket, sentencia, SENTENCIA_PLANIFICADOR_RESP, ID_PLANIFICADOR);
                                //sentencia->Resultado = OK;
                                if (Validar_Posesion_Key(header->IdSolicitante, sentencia))
                                {
                                    sentencia->Resultado = OK;
                                    Liberar_Clave(clave);
                                    free(clave);
                                    Enviar_Sentencia(escucharSocket, sentencia, SENTENCIA_PLANIFICADOR_RESP, header->IdSolicitante);
                                }
                                else
                                {
                                    sentencia->Resultado = ABORTAR;
                                    Enviar_Sentencia(escucharSocket, sentencia, SENTENCIA_PLANIFICADOR_RESP, header->IdSolicitante);
                                    Abortar_ESI_Ejecucion(false);
                                    //int socket_ESI = ((Pid_Socket * ) list_find(lista_Pid_Socket, Es_Socket_Pid_Ejecucion))->Socket;
                                    //Eliminar_Socket_Ejecucion(socket_ESI);
                                    log_error(logger, "El ESI %d fue abortado", header->IdSolicitante);
                                    Planificar();
                                    printf("PAS POR ACAAAAAAAAA\n");
                                    Ejecutar_ESI_Listo();
                                }
                                printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
                            }
                            break;
                            default:
                                break;
                            }
                            free(sentencia);
                            break;
                        }
                        case ESI_OK:
                        {
                            Sentencia *sentencia = Recibir_Sentencia(escucharSocket);
                            printf("Operacion:%d\nClave:%s\nValor:%s\nResultado:%d\n\n", sentencia->Operacion, sentencia->Clave, sentencia->Valor, sentencia->Resultado);
                            if (sentencia->Resultado == OK)
                            {
                                Actualizar_Datos_ESI_Ejecucion();
                                Actualizar_Reloj();
                                enEjecucion = false;
                                if (algoritmo_Planificacion == SJF_CD && !list_is_empty(lista_Pid_Listos) && !consolaCorriendo) {
                                    printf("PASE POR ACAAAA\n");
                                    Planificar();
                                    Ejecutar_ESI_Listo();;
                                }
                                else {
                                    Mandar_Ejecutate_ESI_Ejecucion();   
                                }
                            }
                            else
                            {
                                if(!consolaCorriendo) {   
                                Mandar_Ejecutate_ESI_Ejecucion();
                                }
                            }
                            free(sentencia);
                            break;
                        }
                        case ABORTAR_ESI:
                        {
                            //ENC - cambio el workflow, si se aborta un esi por coordinador, pasa primero por el planificador y lo aborta con el comando kill
                            Sentencia *sentencia = Recibir_Sentencia(escucharSocket);
                            printf("ASDASDA \n");
                            //com_Kill(header->IdSolicitante);
                            printf("SERVIDOR: %d SOCKET ESCUCHADO %d", servidor, header->IdSolicitante);
                            Abortar_ESI_Ejecucion(false);
                            printf("ASDASDA \n");
                            log_error(logger, "El ESI %d fue abortado\n", header->IdSolicitante);
                            Planificar();
                            Ejecutar_ESI_Listo();
                            free(sentencia);
                        }
                        break;
                        default:
                            break;
                        }
                        Destruir_Header(header);
                    }
                }
            }
        }
    }
}

/***************************************************************************
                                    
                            FUNCIONES PRIVADAS DE LA CONSOLA
*****************************************************************************/

int list_index(t_list *list, int pid, int opcion)
{
    int index = -1;
    t_link_element *aux = NULL;
    aux = list->head;
    Header *header = NULL;
    switch (opcion)
    {
    case PIDRECURSO:
    case PIDBLOQUEADOS:
        for (int i = 0; i < list_size(list); i++)
        {
            if (((Pid_Key *)aux->data)->Pid == pid)
            {
                index = i;
                return index;
            }
            else
            {
                aux = aux->next;
            }
        }
        break;
    case PIDSOCKET:
        for (int i = 0; i < list_size(list); i++)
        {
            if (((Pid_Socket *)aux->data)->Pid == pid)
            {
                index = i;

                //Send_Payload(((Pid_Socket *)list_get(lista_Pid_Socket, i))->Socket, ABORTAR_ESI, SIN_PAYLOAD, SIN_DATOS);
                    header = Crear_Header(ABORTAR_ESI, 0, ID_PLANIFICADOR);
                    write(((Pid_Socket *)list_get(lista_Pid_Socket, i))->Socket, header, sizeof(header));
                    free(header);
                   close(((Pid_Socket *)list_get(lista_Pid_Socket, i))->Socket);
                    FD_CLR(((Pid_Socket *)list_get(lista_Pid_Socket, i))->Socket, &master);
                    Finalizar_ESI(pid);
                
                //Eliminar_Socket(();
                return index;
            }
            else
            {
                aux = aux->next;
            }
        }
        break;
    case PIDESIPLAN:
        for (int i = 0; i < list_size(list); i++)
        {
            if (((ESI_Plan *)aux->data)->Pid == pid)
            {
                index = i;
                return index;
            }
            else
            {
                aux = aux->next;
            }
        }
        break;
    default:
        for (int i = 0; i < list_size(list); i++)
        {
            if (((PID)list->head->data) == pid)
            {
                index = i;
                return index;
            }
            else
            {
                aux = aux->next;
            }
        }
        break;
    }
    return index;
}

void Eliminar_PID_Lista(t_list *list, int pid, int opcion)
{
    int index;
    index = list_index(list, pid, opcion);
    if (index != -1)
    {
        Eliminar_Elemento_Lista(index, opcion);
        me_mori = true;
    }
}

void Eliminar_Elemento_Lista(int index, int opcion)
{

    t_list *aux = NULL;
    t_list *aux2 = NULL;
    switch (opcion)
    {
    case PIDSOCKET:
        aux = list_duplicate(lista_Pid_Socket);
        break;
    case PIDRECURSO:
        aux = list_duplicate(lista_Pid_Recurso);
        break;
    case PIDBLOQUEADOS:
        aux = list_duplicate(lista_Pid_Bloqueados);
        break;
    case PIDEJECUCION:
        aux = list_duplicate(lista_Pid_Ejecucion);
        break;
    case PIDLISTOS:
        aux = list_duplicate(lista_Pid_Listos);
        break;
    case PIDNUEVOS:
        aux = list_duplicate(lista_Pid_Nuevos);
        break;
    case PIDDESBLOQUEADOS:
        aux = list_duplicate(lista_Pid_Desbloqueados);
        break;
    case PIDESIPLAN:
        aux = list_duplicate(lista_ESI_Planificacion);
        break;
    }
    aux2 = aux;
    list_remove(aux, index);
    switch (opcion)
    {
    case PIDSOCKET:
        lista_Pid_Socket = aux2;
        break;
    case PIDRECURSO:
        lista_Pid_Recurso = aux2;
        break;
    case PIDBLOQUEADOS:
        lista_Pid_Bloqueados = aux2;
        break;
    case PIDEJECUCION:
        lista_Pid_Ejecucion = aux2;
        break;
    case PIDLISTOS:
        lista_Pid_Listos = aux2;
        break;
    case PIDNUEVOS:
        lista_Pid_Nuevos = aux2;
        break;
    case PIDDESBLOQUEADOS:
        lista_Pid_Desbloqueados = aux2;
        break;
    case PIDESIPLAN:
        lista_ESI_Planificacion = aux2;
        break;
    }
}

bool Se_Encuentra_Pid_A_Borrar_Por_Pid_Key(void *pid_Key)
{
    return ((Pid_Key *)pid_Key)->Pid == pid_a_borrar;
}

bool Clave_A_Desbloquear(void *pid_Key)
{
    //printf("Clave desbloqueable: %s \n\n", ((Pid_Key *)pid_Key)->Key);
    return strcmp(claveADesbloquear, ((Pid_Key *)pid_Key)->Key) == 0;
}

bool Clave_Bloqueada(void *pid_Key)
{
    return strcmp(claveRecurso, ((Pid_Key *)pid_Key)->Key) == 0;
}

bool Clave_A_Bloquear_Por_Pid(void *pid_Key)
{
    return ((int)pid_Key) == pid_A_Bloquear;
}

bool Deadlock_Pid_Match(void *pid_Key)
{
    return ((Pid_Key *)pid_Key)->Pid == Deadlock_Pid;
}

bool Deadlock_Pid_Final(void *pid_Key)
{
    return ((int)pid_Key) == Deadlock_Pid;
}

bool Deadlock_Clave_Tomada_Index(void *pid_Key)
{
    return strcmp(DeadLock_Clave_Index, ((Pid_Key *)pid_Key)->Key) == 0;
}

bool DeadLock_Clave_Tomada_Match(void *pid_Key)
{
    return strcmp(Deadlock_Clave_Match, ((Pid_Key *)pid_Key)->Key) == 0;
}

bool Clave_A_Desbloquear_Por_Pid(void *pid_Key)
{
    return ((Pid_Key *)pid_Key)->Pid == pid_A_Desbloquear;
}

void Destruir_Pid_Key(void *pid_Key)
{
    free(((Pid_Key *)pid_Key)->Key);
    free(pid_Key);
}

void imprimir_Pid_Key(t_list *lista)
{
    t_link_element *aux = NULL;
    int largo;
    aux = lista->head;
    largo = lista->elements_count;
    for (int i = 0; i < largo; i++)
    {
        printf("\tProceso ESI N°: %d Recurso: %s\n\n", ((Pid_Key *)aux->data)->Pid, ((Pid_Key *)aux->data)->Key);
        if (aux->next != NULL)
        {
            aux = aux->next;
        }
    }
}

void imprimir_Pid(t_list *lista)
{
    t_link_element *aux = NULL;
    int largo;
    aux = lista->head;
    largo = lista->elements_count;
    for (int i = 0; i < largo; i++)
    {
        printf("\tProceso ESI N°: %d \n\n", ((PID)aux->data));
        if (aux->next != NULL)
        {
            aux = aux->next;
        }
    }
}

void imprimir_Pid_Socket(t_list *lista)
{
    t_link_element *aux = NULL;
    int largo;
    aux = lista->head;
    largo = lista->elements_count;
    for (int i = 0; i < largo; i++)
    {
        printf("\tProceso ESI N°: %d Socket: %d\n\n", ((Pid_Socket *)aux->data)->Pid, ((Pid_Socket *)aux->data)->Socket);
        if (aux->next != NULL)
        {
            aux = aux->next;
        }
    }
}

void valorClave_instanciaActual(char *clave)
{
    Sentencia *sentencia = Crear_MockSentencia(S_GET, clave, "");
    Enviar_Sentencia(socket_Coordinador, sentencia, VALOR_CLAVE, SIN_ID_SOLICITANTE);
    sentencia = NULL;
    Header *header = Recibir_Header(socket_Coordinador);
    sentencia = Recibir_Sentencia(socket_Coordinador);
    
    if(strlen(sentencia->Valor)>0){
        printf("\t 2. Clave -> Valor: %s -> %s\n", clave, sentencia->Valor);
    }
    else{
        printf("\t 2. La clave %s no tiene cargado valor.\n", clave);
    }

    printf("----------------------------------------\n");
    
    if ((header->IdSolicitante) != ID_COORDINADOR)
    {
        printf("\t 3. La clave %s se encuentra en la instancia %d\n", clave, header->IdSolicitante);
    }

    free(sentencia);
    Destruir_Header(header);
}

void instanciaPosible(char *clave)
{
    Sentencia *sentencia = Crear_MockSentencia(S_GET, clave, "");
    printf("1\n");
    Enviar_Sentencia(socket_Coordinador, sentencia, POSIBLE_INSTANCIA, SIN_ID_SOLICITANTE);
    sentencia = NULL;
    printf("2\n");
    Header *header = Recibir_Header(socket_Coordinador);
    sentencia = Recibir_Sentencia(socket_Coordinador);
    printf("3\n");
    if (header->IdSolicitante != ID_COORDINADOR)
    {
        printf("\t 4. Si se ejecutara la clave %s, se guardaría en la instancia %d \n", clave, header->IdSolicitante);
    }
    printf("4\n");
    free(sentencia);
    printf("5\n");
    Destruir_Header(header);
    printf("6\n");
}

mensaje_deadlock *enviar_mensaje(int pid_dest)
{
    mensaje_deadlock *mensaje = malloc(sizeof(mensaje_deadlock));
    mensaje->pid_inicial = Deadlock_Pid_Inicial;
    mensaje->pid_remitente = Deadlock_Pid;
    mensaje->pid_destinatario = pid_dest;
    return mensaje;
}

bool analizar_mensaje(mensaje_deadlock *mensaje)
{
    return (mensaje->pid_inicial) == (mensaje->pid_destinatario);
}

void Inicializar_Lista_Pid_Recurso(t_list *lista, char **claves)
{
    int i = 0;
    while (claves[i] != NULL)
    {
        Pid_Key *pid_Key = malloc(sizeof(Pid_Key));
        pid_Key->Pid = SISTEMA;
        pid_Key->Key = malloc(sizeof(claveABloquear) + 1);
        strcpy(pid_Key->Key, claves[i]);
        list_add(lista, (void *)pid_Key);
        i++;
    }
}

/***************************************************************************
                                    
                                FUNCIONES CONSOLA
*****************************************************************************/
void com_Pause()
{
    if (!planificadorPausado && reloj != 0 && enEjecucion) {
        pthread_mutex_lock(&sem_ejecucion);
        printf("1\n");
        consolaCorriendo = true;
        pthread_mutex_lock(&sem_ejecucion);
        printf("2\n");
    }
    consolaCorriendo = true;
    planificadorPausado = true;
    if (activarPlanificacion) {
    printf("NO ME TRABEE PUTIN \n");
    activarPlanificacion = false;
    log_info(logger, "Se ha pausado la Planificacion Actual");
    
    }
    else {
        log_info(logger, "La planificación ya se encuentra pausada");
    }
    consolaCorriendo = false;
    pthread_mutex_unlock(&sem_ejecucion);
}

bool Es_Ejecucion_Valida() {
    pid_A_Buscar = pid_Ejecucion;
    return !list_any_satisfy(lista_Pid_Finalizados, Es_Pid_Buscado) && reloj > 0  && !list_any_satisfy(lista_Pid_Bloqueados, Es_Clave_Pid_Recurso_Por_Pid_A_Buscar);
}

void com_Continue()
{
    if (!activarPlanificacion) {
        if (!planificadorPausado) {
            pthread_mutex_lock(&sem_ejecucion);
            printf("10\n");
            consolaCorriendo = true;
            pthread_mutex_lock(&sem_ejecucion);
            printf("11\n");
        }
        consolaCorriendo = true;
        activarPlanificacion = true;
        if (!Es_Ejecucion_Valida()) {
        Planificar();   
        }
        if (Es_Ejecucion_Valida() && algoritmo_Planificacion != SJF_CD) {
            Mandar_Ejecutate_ESI_Ejecucion();
        }
        else {
            Ejecutar_ESI_Listo();
        }
        log_info(logger, "Continuamos con la Planificacion Actual.");
    }
    else {
        log_info(logger, "La planificación ya se encuentra en ejecución");
    }
    planificadorPausado = false;
    pthread_mutex_unlock(&sem_ejecucion);
}

void com_Kill(int ID)
{
    if(!planificadorPausado && reloj != 0 && enEjecucion) {
        pthread_mutex_lock(&sem_ejecucion);
        consolaCorriendo = true;
        pthread_mutex_lock(&sem_ejecucion);
    }
    consolaCorriendo = true;
    pid_a_borrar = ID;
    me_mori = false;

    //Eliminamos de Ejecucion
    Eliminar_PID_Lista(lista_Pid_Ejecucion, pid_a_borrar, PIDEJECUCION);

    //Eliminamos de Desbloqueados
    Eliminar_PID_Lista(lista_Pid_Desbloqueados, pid_a_borrar, PIDDESBLOQUEADOS);

    //Eliminamos de Nuevos
    Eliminar_PID_Lista(lista_Pid_Nuevos, pid_a_borrar, PIDNUEVOS);

    //Eliminamos de Listos
    Eliminar_PID_Lista(lista_Pid_Listos, pid_a_borrar, PIDLISTOS);

    //Eliminamos de Bloqueados
    Eliminar_PID_Lista(lista_Pid_Bloqueados, pid_a_borrar, PIDBLOQUEADOS);

    //Eliminamos dfe Socket
    Eliminar_PID_Lista(lista_Pid_Socket, pid_a_borrar, PIDSOCKET);

    //Eliminamos de ESI_PLAN
    Eliminar_PID_Lista(lista_ESI_Planificacion, pid_a_borrar, PIDESIPLAN);

    //Eliminamos de Recursos
    for (int i = 0; i < list_size(lista_Pid_Recurso); i++)
    {
        Eliminar_PID_Lista(lista_Pid_Recurso, pid_a_borrar, PIDRECURSO);
    }

    if (me_mori)
    {
        log_info(logger, "Se ha eliminado el proceso ESI con ID: %i", ID);
        //Agregamos a Finalizados
        pid_A_Buscar = pid_a_borrar;
        if (!list_any_satisfy(lista_Pid_Finalizados, Es_Pid_Buscado)) {
        list_add(lista_Pid_Finalizados, (void *)pid_a_borrar);
        }
    }
    else
    {
        log_info(logger, "No existe ESI con ID: %i", ID);
    }
    consolaCorriendo = false;
    pthread_mutex_unlock(&sem_ejecucion);
}

void com_Lock(char *clave, int ID)
{
    if (!planificadorPausado && reloj != 0 && enEjecucion) {
        int try = pthread_mutex_trylock(&sem_ejecucion);
        if (try == 0) {
            consolaCorriendo = true;
            pthread_mutex_lock(&sem_ejecucion);
        }
    }
    consolaCorriendo = true;
    /* Se bloqueará el proceso ESI, especificado por dicho ID en la cola del recurso clave.
       Solo se podrán bloquear, ESIs que estén en el estado de listo o ejecutando.
       Vale recordar que cada línea del script a ejecutar es atómica, y no podrá ser interrumpida*/
    pid_A_Bloquear = ID;
    strcpy(claveABloquear, clave);
    bool elPidEstaEnEjecucion = list_any_satisfy(lista_Pid_Ejecucion, Clave_A_Bloquear_Por_Pid);
    bool elPidEstaEnListos = list_any_satisfy(lista_Pid_Listos, Clave_A_Bloquear_Por_Pid);
    Pid_Key *pid_Key = malloc(sizeof(Pid_Key));
    pid_Key->Pid = ID;
    pid_Key->Key = malloc(sizeof(strlen(claveABloquear) + 1));
    strcpy(pid_Key->Key, claveABloquear);
    if (elPidEstaEnEjecucion)
    {
        list_remove_by_condition(lista_Pid_Ejecucion, Clave_A_Bloquear_Por_Pid);
        list_add(lista_Pid_Bloqueados, (void *)pid_Key);
        log_info(logger, "Se ha bloqueado el proceso ESI con ID: %i en la cola de la clave: %s", ID, clave);
    }
    else if (elPidEstaEnListos)
    {
        list_remove_by_condition(lista_Pid_Listos, Clave_A_Bloquear_Por_Pid);
        list_add(lista_Pid_Bloqueados, (void *)pid_Key);
        log_info(logger, "Se ha bloqueado el proceso ESI con ID: %i en la cola de la clave: %s", ID, clave);
    }
    else
    {
        log_info(logger, "Usted ha intentado bloquear el proceso ESI con ID: %i en la cola de la clave: %s", ID, clave);
        log_info(logger, "Solo se podrán bloquear procesos ESIs que estén en el estado de listo o ejecucion.");
    }
    Destruir_Pid_Key(pid_Key);
    consolaCorriendo = false;
    pthread_mutex_unlock(&sem_ejecucion);
}

void com_Unlock(char *clave)
{
    if (!planificadorPausado && reloj != 0 && enEjecucion) {
        int try = pthread_mutex_trylock(&sem_ejecucion);
        if (try == 0) {
            consolaCorriendo = true;
            pthread_mutex_lock(&sem_ejecucion);
        }
    }
    consolaCorriendo = true;
    /*Se desbloqueara el primer proceso ESI bloquedo por la clave especificada, 
    en caso de que no queden mas procesos bloqueados se debera liberar la clave.*/
    strcpy(claveADesbloquear, clave);
    Pid_Key *pid_Key_Bloqueado = (Pid_Key *)list_find(lista_Pid_Bloqueados, Clave_A_Desbloquear);
    if (pid_Key_Bloqueado == NULL)
    {
        log_info(logger, "No hay ningun ESI con la clave %s", clave);
    }
    else
    {
        pid_A_Desbloquear = pid_Key_Bloqueado->Pid;
        log_info(logger, "Se desbloqueo el Pid: %d con la clave %s", pid_A_Desbloquear, clave);
        //list_remove_and_destroy_by_condition(lista_Pid_Bloqueados, Clave_A_Desbloquear, Destruir_Pid_Key);
        list_remove_by_condition(lista_Pid_Bloqueados, Clave_A_Desbloquear);
        //list_remove_and_destroy_by_condition(lista_Pid_Recurso, Clave_A_Desbloquear_Por_Pid, Destruir_Pid_Key);
        list_remove_by_condition(lista_Pid_Recurso, Clave_A_Desbloquear_Por_Pid);
        list_add_in_index(lista_Pid_Desbloqueados, list_size(lista_Pid_Desbloqueados), (void *)pid_A_Desbloquear);
    }

    pid_Key_Bloqueado = (Pid_Key *)list_find(lista_Pid_Recurso, Clave_A_Desbloquear);
    if (pid_Key_Bloqueado != NULL && pid_Key_Bloqueado->Pid == SISTEMA)
    {
        list_remove_by_condition(lista_Pid_Recurso, Clave_A_Desbloquear);
        free(pid_Key_Bloqueado);
        log_info(logger, "Liberada la clave %s que fue bloqueada por configuración", claveADesbloquear);
    }

    Pid_Key *pid_Key = list_find(lista_Pid_Recurso, Clave_A_Desbloquear);
    if (pid_Key == NULL)
    {
        log_info(logger, "Se desbloqueo la clave %s", clave);
    }
    else
    {
        log_info(logger, "La clave %s sigue bloqueada", clave);
    }    

    if (reloj > ultimo_Reloj_ESI_Estimado)
    {
        Estimar_Tiempo_ESI(pid_Ejecucion);
    }
    if (algoritmo_Planificacion == SJF_CD || list_is_empty(lista_Pid_Listos))
    {
        Planificar();
    }


   consolaCorriendo = false;
   if (Es_Ejecucion_Valida() && algoritmo_Planificacion != SJF_CD) {
       Mandar_Ejecutate_ESI_Ejecucion();
   } 
   else {
       Ejecutar_ESI_Listo();
   }
    pthread_mutex_unlock(&sem_ejecucion);
}

void com_Status(char *clave)
{

    com_List(clave);
    printf("----------------------------------------\n");
    valorClave_instanciaActual(clave);
    printf("----------------------------------------\n");
    instanciaPosible(clave);
}

void com_List(char *recurso)
{
    strcpy(claveRecurso, recurso);
    t_list *nueva_lista_bloq = list_filter(lista_Pid_Bloqueados, Clave_Bloqueada);
    if(!list_is_empty(nueva_lista_bloq)){
        printf("\t 1. ESIs bloqueados por la clave %s\n", recurso);
        imprimir_Pid_Key(nueva_lista_bloq);
    }
    printf("\t 1. No existen ESIs bloqueados para la clave %s\n", recurso);
}

void com_Imprimir()
{

    printf("Lista de ESIs Bloqueados: \n");
    imprimir_Pid_Key(lista_Pid_Bloqueados);
    printf("----------------------------------------\n");
    printf("Lista de ESIs Desbloqueados: \n");
    imprimir_Pid(lista_Pid_Desbloqueados);
    printf("----------------------------------------\n");
    printf("Lista de ESIs con Recursos: \n");
    imprimir_Pid_Key(lista_Pid_Recurso);
    printf("----------------------------------------\n");
    printf("Lista de ESIs Finalizados: \n");
    imprimir_Pid(lista_Pid_Finalizados);
    printf("----------------------------------------\n");
    printf("Lista de ESIs Listos: \n");
    imprimir_Pid(lista_Pid_Listos);
    printf("----------------------------------------\n");
    printf("Lista de ESIs Nuevos: \n");
    imprimir_Pid(lista_Pid_Nuevos);
    printf("----------------------------------------\n");
    printf("Lista de ESI en Ejecucion: \n");
    pid_A_Buscar = pid_Ejecucion;
    if (enEjecucion || (algoritmo_Planificacion == SJF_CD && !Es_Ejecucion_Valida())) {
        printf("\tProceso ESI N°: %d \n\n", pid_Ejecucion);
    }
    printf("----------------------------------------\n");
    printf("Lista de ESIs con su Socket: \n");
    imprimir_Pid_Socket(lista_Pid_Socket);
    printf("----------------------------------------\n");
}

void com_Deadlock()
{

    int index;
    t_list *lista_Deadlock = list_create();
    t_list *lista_Deadlock_Definitiva = list_create();
    t_list *nueva_lista_rec_bloq = list_create();
    Pid_Key *pid_Key1 = NULL, *pid_Key2 = NULL, *pid_Key3 = NULL, *pid_Key_DeadLock1 = NULL, *pid_Key_DeadLock2 = NULL;

    for (index = 0; index < list_size(lista_Pid_Bloqueados); index++)
    {
        pid_Key1 = (Pid_Key *)list_get(lista_Pid_Bloqueados, index);
        Deadlock_Pid = pid_Key1->Pid;
        pid_Key2 = list_find(lista_Pid_Recurso, Deadlock_Pid_Match);
        if (pid_Key2 != NULL)
        {
            list_add(nueva_lista_rec_bloq, (void *)pid_Key2);
        }
    }

    mensaje_deadlock *mensaje = malloc(sizeof(mensaje_deadlock));
    for (int index = 0; index < list_size(lista_Pid_Bloqueados); index++)
    {
        pid_Key_DeadLock1 = (Pid_Key *)list_get(lista_Pid_Bloqueados, index);
        int deadlock = 0;
        bool match = false;
        strcpy(Deadlock_Clave_Match, pid_Key_DeadLock1->Key);
        Deadlock_Pid = pid_Key_DeadLock1->Pid;
        Deadlock_Pid_Inicial = pid_Key_DeadLock1->Pid;

        while (!deadlock)
        {
            pid_Key_DeadLock2 = list_find(nueva_lista_rec_bloq, DeadLock_Clave_Tomada_Match);
            if (pid_Key_DeadLock2 != NULL)
            {
                list_add(lista_Deadlock, (void *)Deadlock_Pid);
                mensaje = enviar_mensaje(pid_Key_DeadLock2->Pid);
                match = analizar_mensaje(mensaje);
                Deadlock_Pid = pid_Key_DeadLock2->Pid;
                if (match)
                {
                    list_add(lista_Deadlock, (void *)Deadlock_Pid);
                    for (int b = 0; b < list_size(lista_Deadlock); b++)
                    {
                        pid_A_Bloquear = (int)list_get(lista_Deadlock, b);
                        bool hay_repetido = list_any_satisfy(lista_Deadlock_Definitiva, Clave_A_Bloquear_Por_Pid);
                        if (!hay_repetido)
                        {
                            list_add(lista_Deadlock_Definitiva, (void *)pid_A_Bloquear);
                        }
                    }
                    deadlock = 1;
                }
                else
                {
                    list_add(lista_Deadlock, (void *)Deadlock_Pid);
                    pid_Key3 = list_find(lista_Pid_Bloqueados, Deadlock_Pid_Match);
                    strcpy(Deadlock_Clave_Match, pid_Key3->Key);
                }
            }
            else
            {
                deadlock = 1;
            }
        }
    }
    if (!list_is_empty(lista_Deadlock_Definitiva))
    {
        imprimir_Pid(lista_Deadlock_Definitiva);
    }
    else
    {
        printf("No se encuentran procesos en Deadlock.\n");
    }
    list_destroy(nueva_lista_rec_bloq);
    list_destroy(lista_Deadlock);
    free(mensaje);
}

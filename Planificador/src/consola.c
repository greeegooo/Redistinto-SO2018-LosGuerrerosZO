#include "../includes/consola.h"

COMMAND commands[] = {
        { "Pause", "\tEl Planificador queda pausado y no da nuevas ordenes de ejecucion." },
        { "Continue", "\tEl Planificador vuelve a estar activo en caso de pausar su funcionamiento." },
        { "Lock", "\tSe bloquea un proceso ESI con el ID especificado y su clave." },
        { "Unlock","\tSe desbloquea un proceso ESI con la clave especificado." },
        { "List", "\tLista los procesos ESIs que esperan un recurso en particular." },
        { "Kill", "\tSe finaliza un proceso." },
        { "Status", "\tSe otorga informacion sobre las instancias que poseen cierta clave." },
        { "Deadlock", "\tAnaliza los Deadlocks existentes." },
        { "Help", "\tLe muestra al usuario los comandos que puede implementar en esta consola." },
        { "Exit", "\tSe sale de la Consola. Se aborta el Planificador" },
        { "Imprimir", "\tImprime las listas." },
        { "Clear", "\tLimpia la pantalla." }
    };
  
void * consolaPlan(void * unused){
    char* linea;
    char separador [] = " ";

    printf("\n\tBienvenido a la Consola del Planificador.\n");
    printf("\nSi desea conocer la lista de comandos, utilice el comando Help. \n");

    //rl_catch_sigwinch = 0;
    //rl_catch_signals = 0;
    
    while (1) {
        char *token = NULL;
        char comando [20];
        linea = readline("> ");
        add_history(linea);
        token = strtok(linea, separador);
        
        strcpy(comando,token);
        convertir(comando);

        if(!strncmp(comando, "EXIT", 4)) {
            free(linea);
            GOODBYE_WORLD();
            break;
        }

        if(!strncmp(comando, "HELP", 4)) {
            for (int i = 0; i < 12; i++) {
              printf("%d. %s, %s\n", i+1,commands[i].name, commands[i].doc);
            }
            free(linea);
        }

        if(!strncmp(comando, "KILL", 4)) {
            token =strtok(NULL, separador);
            if(token!=NULL){
              int ID = atoi(token);
              com_Kill(ID);
            } else{
              printf("El comando Kill debe presentar un parametro.\n");
            }
            free(linea);
        }

        if(!strncmp(comando, "LOCK", 4)) {
          token =strtok(NULL, separador);
          if(token!=NULL){
            char *clave = token;
            token =strtok(NULL, separador);
            if(token!=NULL){
              int ID = atoi(token);
              com_Lock(clave, ID);
            } else{
              printf("Le falta un parametro mas al comando Lock\n");
            }
          }else{
            printf("El comando Lock debe presentar 2 parametros.\n");
          }
          free(linea);
        }

        if(!strncmp(comando, "UNLOCK", 6)) {
          token =strtok(NULL, separador);
          if(token!=NULL){
            char *clave = token;
            com_Unlock(clave);
          }else{
            printf("El comando Unlock debe presentar un parametro\n");
          }
          free(linea);
        }

        if(!strncmp(comando, "STATUS", 6)) {
          token =strtok(NULL, separador);
          if(token!=NULL){
            char *clave = token;
            com_Status(clave);
          }else{
            printf("El comando Status debe presentar un parametro\n");
          }
          free(linea);
        }

        if (!strncmp(comando, "PAUSE", 5)) {
          com_Pause();
          free(linea);
        }

        if (!strncmp(comando, "CONTINUE", 8)) {
          com_Continue();
          free(linea);
        }

        if (!strncmp(comando, "DEADLOCK", 8)) {
          com_Deadlock();
          free(linea);
        }

        if (!strncmp(comando, "CLEAR", 5)) {
          fprintf(stdout, "\33[2J");
          fprintf(stdout, "\33[1;1H"); //posiciono el cursor en la primer columna.
          printf("\n\tBienvenido a la Consola del Planificador.\n");
          printf("\nSi desea conocer la lista de comandos, utilice el comando Help. \n");
          free(linea);
        }

        if (!strncmp(comando, "LIST", 4)) {
          token =strtok(NULL, separador);
          if(token != NULL){
            char *clave = token;
            com_List(clave);
          }else{
            printf("Elcomando List debe presentar un parametro\n");
          }
          free(linea);
        }

        if(!strncmp(comando, "IMPRIMIR", 8)) {
          com_Imprimir();
          free(linea);
        }
    }
    return 0;
}

void convertir(char *palabra)	{
	int i;
	for( i=0; i<strlen(palabra); i++){
      palabra[i]=toupper(palabra[i]);
  }
}

#include "includes/cliente.h"

void _exit_with_error(t_log * logger, int socket, char* error_msg, void * buffer) {

  if (buffer != NULL) free(buffer);
  log_error(logger, error_msg);
  close(socket);

  exit_gracefully(logger, 1);
}

void exit_gracefully(t_log * logger, int return_nr) {
  
  log_destroy(logger);
  exit(return_nr);
}

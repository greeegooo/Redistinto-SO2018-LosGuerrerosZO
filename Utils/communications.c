#include "includes/communications.h"

int connect_to_server(t_log * logger, char * ip, char * port) {

    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    
    hints.ai_socktype = SOCK_STREAM;  

    getaddrinfo(ip, port, &hints, &server_info);

    int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    int res = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

    freeaddrinfo(server_info);  

    if (res < 0) _exit_with_error(logger, server_socket, "connect_to_server(): No se pudo conectar al servidor", NULL);
    
    return server_socket;
}

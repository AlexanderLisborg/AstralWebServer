#include "socket_helper.h"
#include "astral_error.h"

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

// Read one http request into buffer. Return 0 on success, 
// -1 on error.
int read_next_http_request(int const fd, char* const buffer,unsigned int const bufferSize ,unsigned int const nrOfBytesToRead){
    // TODO: Fix this implementation...
    // * It currently just reads the entire socket, if multiple
    // requests are in the socket, it should read one at a time.
    // * Payload should be disregarded when reading requests.
    // Perhaps respond with error page if payload is present.
    // * Handle read failure gracefully and return astral error
    // code. Exit if error cannot be handled.
    int suc = read(fd,buffer,nrOfBytesToRead);
    enforce(suc>=0,"Failed to read from connection socket.");
    printf("Read %d bytes! \n",suc);
}

// Accept next incoming tcp connection (blocking). Return fd
// of the instantiated tcp connection socket on success, -1 on
// failure.
int accept_tcp_connection(int const listenerSocketFd){
    //TODO : return -1 or handle errors instead of enforce.
    struct sockaddr_in clientAddress;
    int clientLength = sizeof(clientAddress);
    int connectionFd = accept(listenerSocketFd,(sockaddr_t *) &clientAddress,(socklen_t *)&clientLength);
    enforce(connectionFd>=0,"Failed to instantiate tcp connection socket.");
    return connectionFd;
}

// Instantiate tcp listener with hard-coded options. 
// Return fd of the instantiated tcp listener socket on success,
// -1 on failure.
int instantiate_tcp_listener(int const port){
    // TODO : return -1 or handle errors instead of enforce.
    int listenerSocketFd = socket(AF_INET,SOCK_STREAM,0);
    enforce(listenerSocketFd>=0,"Failed to instantiate tcp listener socket.");
    int optionValue = 1;
    int suc = setsockopt(listenerSocketFd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optionValue, sizeof(int));
    enforce(suc>=0,"Failed to initialize tcp listener socket options.");
    struct sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_port = htons((unsigned short) port);
    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    suc = bind(listenerSocketFd, (struct sockaddr *) &srv, sizeof(srv));
    enforce(suc>=0,"Failed to bind tcp listener socket.");
    suc = listen(listenerSocketFd,1024);
    enforce(suc<=0,"Tcp listener socket failure while waiting for connections.");
    return (unsigned int) listenerSocketFd;
}
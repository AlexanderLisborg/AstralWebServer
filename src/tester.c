
#include "socket_helper.h"
#include "http_helper.h"
#include "astral_error.h"

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

typedef struct sockaddr sockaddr_t;

#define MAX_HTTP_REQUEST_SIZE 8192

int main(){
    int const port = 10000;
    printf("Hello world.\n");
    printf("haa%s\n","lol");
    
    int listenerSocketFd = instantiate_tcp_listener(port);

    int connectionSocketFd = accept_tcp_connection(listenerSocketFd);

    char readBuffer[MAX_HTTP_REQUEST_SIZE];
    int suc = read(connectionSocketFd,readBuffer,MAX_HTTP_REQUEST_SIZE);
    printf("Read %d bytes! \n",suc);
    readBuffer[MAX_HTTP_REQUEST_SIZE-1] = '\0';
    printf("Read : %s \n",readBuffer);
    suc = chdir("../html");
    enforce(suc==0,"Failed change dir.");
    handle_http_request(connectionSocketFd,readBuffer);
    return 0;
}
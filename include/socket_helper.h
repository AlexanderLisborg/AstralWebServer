#ifndef __socket_helper__
#define __socket_helper_

typedef struct sockaddr sockaddr_t;

// Read a line from socket file descriptor. 
// Tip: read into a char buffer, make use of read() command.
// If socket is closed prematurely while threads are blocked
// waiting to read it, handle the resulting error from read().
int read_next_http_request(int const fd, char* const buffer,unsigned int const bufferSize ,unsigned int const nrOfBytesToRead);

// Open socket and establish TCP connection with hostname
// and port.
// Tip: use socket() and connect(), sockaddr_in  and sockaddr 
// structs may be relevant
int accept_tcp_connection(int const listenerSocketFd);
// Open socket and wait for incoming TCP connections on port.
// Tip: use socket() and listen(), sockaddr_in  and sockaddr 
// structs may be relevant
int instantiate_tcp_listener(int const port);


#endif // guard
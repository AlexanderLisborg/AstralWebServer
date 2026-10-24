#ifndef __io_helper__
#define __io_helper__

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

typedef struct sockaddr sockaddr_t;

// Read a line from socket file descriptor. 
// Tip: read into a char buffer, make use of read() command.
// If socket is closed prematurely while threads are blocked
// waiting to read it, handle the resulting error from read().
unsigned int read_line_socket(int fd, void* buf, unsigned int maxlen);

// Open socket and establish TCP connection with hostname
// and port.
// Tip: use socket() and connect(), sockaddr_in  and sockaddr 
// structs may be relevant
int open_tcp_connect_fd(char*hostname, int port);

// Open socket and wait for incoming TCP connections on port.
// Tip: use socket() and listen(), sockaddr_in  and sockaddr 
// structs may be relevant
int open_tcp_listen_fd(int port);

// Handle error checking inside macros to allow __LINE__ to 
// display accurate line number when logging. 
#define fork_or_die() ({pid_t pid = fork(); assert(pid>=0);pid;})



#endif // guard
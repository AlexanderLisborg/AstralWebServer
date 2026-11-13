#include <stdio.h>
#include "string.h"
#include "http_helper.h"
#include "socket_helper.h"
#include "job_queue.h"
#include "astral_error.h"
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>


char defaultRoot[] = "../html";

#define MAX_HTTP_REQUEST_SIZE 8192

void job_fun(int,char*);
void *thread_handler(void *);
int request_parse_uri(char *uri, char *filename, char *cgiargs);
void set_socket_timeout(int, int);
 
int main(int argc, char *argv[]) {
    int c;
    char *rootDir = defaultRoot;
    int port = 10000;
	int nrOfThreads = 1;
	int jobQueueCapacity = 1;
	char* logFileName = NULL;
	enum SCHEDALG schedalg = FIFO;
    
    while ((c = getopt(argc, argv, "d:p:t:b:s:")) != -1)
	switch (c) {
	case 'd':
	    rootDir = optarg;
	    break;
	case 'p':
	    port = atoi(optarg);
	    break;
	case 't':
		nrOfThreads = atoi(optarg);
		break;
	case 'b':
		jobQueueCapacity = atoi(optarg);
		break;
	case 's':
		if(!strcmp(optarg,"SFF")){
			schedalg = SFF;
		}
		break;
	case 'l':
		logFileName = optarg;
		break;
	default:
	    fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffers] [-s schedalg]\n");
	    exit(1);
	}
	// Change directory
    enforce(
		chdir(rootDir)==0,
		"Failed change dir."
	); 

    int listenerSocketFd = instantiate_tcp_listener(port);
	// Init worker pool
	job_queue_t *q;
	q = (job_queue_t*)malloc(sizeof(job_queue_t));
	job_queue_init(q,job_fun,jobQueueCapacity,schedalg,logFileName);
	pthread_t threads[nrOfThreads];
	for(int i = 0 ; i < nrOfThreads; i++){
		enforce(
			pthread_create(&threads[i],NULL,&thread_handler,q)==0,
			"Failed creation of thread."
		); 
	}

    while (1) {
		int connectionSocketFd = accept_tcp_connection(listenerSocketFd);
		// Note: Everything after the TCP connection has been established 
		// can be done in worker threads instead of main thread...
		set_socket_timeout(connectionSocketFd,3);

		char *readBuffer = (char*) malloc(MAX_HTTP_REQUEST_SIZE);
    	int readBytes = read(connectionSocketFd,readBuffer,MAX_HTTP_REQUEST_SIZE);
		printf("Read %d bytes : %s \n",readBytes,readBuffer);
		// Blocking enqueue TODO: Pass file size so 
		// Shortest File First scheduling works
		job_queue_enqueue(q,connectionSocketFd,0xFFFFFFFF,readBuffer); 
	}
	return 0;
}
// Job function for worker threads to execute
void job_fun(int fd, char* readBuffer){
	pathmapping_t mapping[] = {{"/","./default.html"}}; 
	pathmap_t pathmap = {mapping,1};
	handle_http_request(fd,readBuffer,&pathmap);
	enforce(
		close(fd),
		"Failed closing socket."
	);
	free(readBuffer);
	return;
};

// Main loop for worker threads. 
void *thread_handler(void *arg){
	job_queue_t *q = (job_queue_t *) arg;
	while(1){
		assert(!job_queue_execute(q)); // Blocked when empty TODO: replace assert with ass 
	}
};
// Straight up kill the socket after some time. (Will cause read errors for blocked
// processeses waiting to read the socket!)
void set_socket_timeout(int fd, int seconds) { 
    struct timeval tv;
    tv.tv_sec  = seconds;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    //setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}
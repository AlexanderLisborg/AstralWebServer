#include <stdio.h>
#include "string.h"
#include "http_helper.h"
#include "io_helper.h"
#include "job_queue.h"
#include <time.h>
#include <unistd.h>


char default_root[] = ".";

void job_fun(int,char*);
void *thread_handler(void *);
int request_parse_uri(char *uri, char *filename, char *cgiargs);
void set_socket_timeout(int, int);
 
int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
	int nrOfThreads = 1;
	int jobQueueCapacity = 1;
	char* logFileName = NULL;
	enum SCHEDALG schedalg = FIFO;
    
    while ((c = getopt(argc, argv, "d:p:t:b:s:")) != -1)
	switch (c) {
	case 'd':
	    root_dir = optarg;
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

	chdir_or_die(root_dir); 

    int listen_fd = open_listen_fd_or_die(port);
	// Init worker pool
	job_queue_t *q;
	q = (job_queue_t*)malloc(sizeof(job_queue_t));
	job_queue_init(q,job_fun,jobQueueCapacity,schedalg,logFileName);
	pthread_t threads[nrOfThreads];
	for(int i = 0 ; i < nrOfThreads; i++){
		assert(!pthread_create(&threads[i],NULL,&thread_handler,q)); // TODO: replace assert with ass 
	}

    while (1) {
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
		// Note: Everything after the TCP connection has been established 
		// can be done in worker threads instead of main thread...
		set_socket_timeout(conn_fd,3);

		char buf[8192], bufMutate[8192];
		char* bufMutateP = bufMutate;
		// If socket was closed or on other unexpected read error, continue
		// TODO: Actually catch the socket closed error through errno, kill process on unknown errors.
		if(readline(conn_fd, buf, 8192)<0) continue;
		strcpy(bufMutateP,buf); // strncpy ?
		int validLine = 1;
		char* pre = strsep(&bufMutateP," ");
		if(bufMutateP==NULL || strcmp(pre,"GET")) validLine = 0;
		char* uri = strsep(&bufMutateP,"& ");
		if(bufMutateP==NULL || uri == '\0') validLine = 0;
		
		if(validLine){
			// Static check is done twice, it should not be done here
			// TODO: Remove redundant static URI check and respond with
			// 404 forbidden when static resources are referenced.
			*(uri-1)='.'; // Interpret all URIs as relative
			uri--;
			char* p = uri;
			if (strstr(uri, "..") != NULL) validLine = 0;
			if(validLine){
				char* line = (char*) calloc(strlen(buf)+1,sizeof(char));
				assert(line!=NULL); // TODO: replace assert with ass 
				strcpy(line,buf); // strncpy ?
				unsigned int fileSize = 0xFFFFFFFF;
				struct stat *statbuf = NULL;
				// If file exists and is readable, extract its size
				if(!access(uri,R_OK)){ 
					statbuf = malloc(sizeof(struct stat));
					assert(statbuf!=NULL); // TODO: replace assert with ass 
					assert(stat(uri, statbuf) == 0); // TODO: replace assert with ass 
					fileSize = statbuf->st_size;
				}

				// Blocking enqueue
				job_queue_enqueue(q,conn_fd,fileSize,line); 
				free(statbuf);
			} else printf("Not valid Double dot\n");
			
		} else printf("Not valid first pass\n");
	}
	return 0;
}
// Job function for worker threads to execute
void job_fun(int fd, char* line){
	request_handle(fd,line);
	close_or_die(fd);
	free(line);
	return;
};

// Main loop for worker threads. 
void *thread_handler(void *arg){
	job_queue_t *q = (job_queue_t *) arg;
	while(1){
		assert(!job_queue_execute(q)); // Blocked when empty TODO: replace assert with ass 
	}
	return;
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
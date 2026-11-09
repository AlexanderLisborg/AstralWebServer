
#include "astral_error.h"
#include "http_helper.h"

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

// HTTP/1.0 syntax sourced from https://www.w3.org/Protocols/HTTP/1.0/spec.html#Request

// Arbitrary, may be subject to change
#define ASTRAL_MAX_URI_LENGTH 1024
#define ASTRAL_MAX_REQUEST_LENGTH 2048
#define ASTRAL_MAX_RESOURCE_SIZE 1073741824 // 1 GB
#define ASTRAL_READ_CHUNK_SIZE 2097152 // 2 MB

void serve_error(int socketFd, astral_error_t error);
int serve_file(int const socketFd, int const fileFd, unsigned int const bufferSize,unsigned int const fileSize);

void parse_http_request(int fd,char* request){
	/* HTTP request BNF syntax 
	Request = Simple-Request | Full-Request
	Simple-Request = "GET" SP Request-URI CRLF
	Full-Request = Request-Line *( General-Header  | Request-Header | Entity-Header ) CRLF [ Entity-Body ]
	Request-Line = Method SP Request-URI SP HTTP-Version CRLF
	General-Header = 

	// Server should send status code 501 if extension-method is not available
	Method = "GET" | "HEAD" | "POST" | extension-method 
	extension-method = token 
	*/
}

void handle_http_GET(int fd,char* uri){

}

int serve_resource(int socketFd, char* path){
	// Check path length
	if(strnlen(path,ASTRAL_MAX_URI_LENGTH)==ASTRAL_MAX_URI_LENGTH){
		serve_error(socketFd,E_URI_TOO_LONG);
		return E_URI_TOO_LONG;
	}
	// Check illegal escape sequence ".."
	if(strstr(path,"..")==NULL){
		serve_error(socketFd,E_URI_ESCAPE_SEQUENCE_FOUND);
		return E_URI_ESCAPE_SEQUENCE_FOUND;
	}
	// Check if path is static (/example/static/uri)
	if(*path!='.'){ 
		serve_error(socketFd,E_URI_IS_STATIC);
		return E_URI_IS_STATIC;
	}
	// Check if file is non-regular (socket,symbolic link, block device, directory, character device, FIFO)
	struct stat statBuffer;

	// TODO: fix rare race condition:
	// between the reading and checking of stat, if the
	// file changes because of external effects, it might 
	// check an outdated value, leading to incorrect behaviour.

	// Aquire stat lock ?
	lstat(path,&statBuffer);
	
	if(!S_ISREG(statBuffer.st_mode)){
		serve_error(socketFd,E_FILE_IS_NON_REGULAR);
		return E_FILE_IS_NON_REGULAR;
	}

	// Check if file is empty
	if(statBuffer.st_size==0){
		serve_error(socketFd,E_FILE_IS_EMPTY);
		return E_FILE_IS_EMPTY;
	}

	// Check if file is too large
	if(statBuffer.st_size>ASTRAL_MAX_RESOURCE_SIZE){
		serve_error(socketFd,E_FILE_TOO_LARGE);
		return E_FILE_TOO_LARGE;
	}

	// Check read access
	if(access(path,R_OK)!=F_OK){
		serve_error(socketFd,E_FILE_READ_ACCESS_DENIED);
		return E_FILE_READ_ACCESS_DENIED;
	}
	
	//Read content, pass into fd.
	int fileFd = open(path,O_RDONLY);
	enforce(fileFd>=0,"Failed opening requested file.");
	int res = serve_file(socketFd,fileFd,ASTRAL_READ_CHUNK_SIZE,statBuffer.st_size);
	if(res != 0){
		serve_error(socketFd,res);
		return res;
	}

	//Release stat lock?

	return 0; //Success
}

void serve_error(int socketFd, astral_error_t error){
	//TODO : Evaluate strings at compile time rather than run time.
	char errorPageFallback[512];
	sprintf(errorPageFallback, ""
		"<!DOCTYPE HTML>\r\n"
		"<html>\r\n"
		"\t<h1>403 Forbidden</h1>\r\n"
		"<!html>\r\n\r\n"
	);
	char headerBuf[512];
	sprintf(headerBuf, ""
		"HTTP/1.0 200 OK\r\n"
		"Server: WebServer\r\n"
		"Content-Length: %u\r\n"
		"Content-Type: text/html\r\n\r\n"
	,strnlen(errorPageFallback,512));
	int sc1 = send(socketFd,headerBuf,strnlen(headerBuf,512),MSG_MORE);
	int sc2 = send(socketFd,errorPageFallback,strnlen(errorPageFallback,512),0);
	// TODO: Handle error instead of crashing
	enforce(sc1==strnlen(errorPageFallback,512), "Failed writing to socket.");
	enforce(sc2==strnlen(errorPageFallback,512), "Failed writing to socket.");
	enforce(close(socketFd)==0,"Failed to close socket.");
}
// Reads file in chunks of bufferSize at a time and sends
// all data to socketFd. Closes socketFd and returns 0
// on success. Returns astral error code on error.
int serve_file(int const socketFd, int const fileFd, unsigned int const bufferSize,unsigned int const fileSize){
	char headerBuf[512];
	//TODO: Evaluate string at compile time rather tan run time.
	sprintf(headerBuf, ""
		"HTTP/1.0 200 OK\r\n"
		"Server: WebServer\r\n"
		"Content-Length: %u\r\n"
		"Content-Type: text/html\r\n\r\n"
	,fileSize);
	int sc = send(socketFd,headerBuf,strnlen(headerBuf,512),MSG_MORE);
	// TODO: Handle error instead of crashing
	enforce(sc==strnlen(headerBuf,512), "Failed writing to socket.");
	char readWriteBuffer[ASTRAL_MAX_REQUEST_LENGTH];
	for(int i = 0; 
		i < (ASTRAL_MAX_RESOURCE_SIZE/ASTRAL_READ_CHUNK_SIZE);
		i++
	){
		int rc = read(fileFd,readWriteBuffer,bufferSize);
		if(rc>0){ // Successful read
			int sc = send(socketFd,readWriteBuffer,rc,MSG_MORE);
			// TODO: Handle failed send instead of crashing
			enforce(sc==rc,"Failed writing to socket.");
			enforce(close(socketFd)==0,"Failed to close socket.");
			continue;
		}
		if(rc==0){ // EOF reached
			return 0; // Success
		}
		else{ // Read error
			return E_FILE_READ_FAILED;
		}
	}
}

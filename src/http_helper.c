
#include "astral_error.h"
#include "http_helper.h"

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

// HTTP/1.0 syntax sourced from https://www.w3.org/Protocols/HTTP/1.0/spec.html#Request

// Arbitrary macro values, may be subject to change
#define ASTRAL_MAX_URI_LENGTH 1024
#define ASTRAL_MAX_REQUEST_LENGTH 2048
#define ASTRAL_MAX_RESOURCE_SIZE 1073741824 // 1 GB
#define ASTRAL_READ_CHUNK_SIZE 2097152 // 2 MB

int _serve_resource(int socketFd, char_span_t uri);
void _serve_error(int socketFd, astral_error_t error);
int _serve_fd(int const socketFd, int const fileFd, unsigned int const bufferSize,unsigned int const fileSize);
void _parse_header(http_header_args_t *header,char* request);

void handle_http_request(int fd, char* request){
	http_header_args_t header = {0};
	_parse_header(&header,request);
	// if GET method with nonempty args, handle.
	if(header.method==GET && 
		header.resourceURI.head!=0 &&
		header.resourceURI.length!=0 &&
		header.versionMajor!=0 &&
		header.versionMinor!=0
	){
		_serve_resource(fd,header.resourceURI);
	}

}

void _parse_header(http_header_args_t *header,char* request){
	int i = 0;
	if(strncmp(request,"GET ",4)==0){
		header->method = GET;
		i+=4;
	}
	int uriStart = i;
	for(; i < ASTRAL_MAX_URI_LENGTH + uriStart ; i++){
		if(request[i]==' '){
			break;
		}
	}
	if(i<=ASTRAL_MAX_URI_LENGTH+uriStart){
		char_span_t sp = {(request + uriStart), i - uriStart};
		header->resourceURI = sp;
	}
	i++;

	if(strncmp((request+i),"HTTP/1.1",8)==0){
		header->versionMajor=1;
		header->versionMinor=1;
		i+=8;
	}
	
}

int _serve_resource(int socketFd, char_span_t uri){
	// Check uri length
	if(uri.length>ASTRAL_MAX_URI_LENGTH){
		_serve_error(socketFd,E_URI_TOO_LONG);
		return E_URI_TOO_LONG;
	}
	// Path is always relative.
	char *path = uri.head-1;
	// Modify uri in place such that it becomes 
	// the relative path C string... This corrupts the uri.
	*(path) = '.';
	*(uri.head + uri.length) = '\0';

	// Check illegal escape sequence ".." in path.
	if((memmem(path, uri.length=1, "..", 2)!=0 )){
		_serve_error(socketFd,E_URI_ESCAPE_SEQUENCE_FOUND);
		return E_URI_ESCAPE_SEQUENCE_FOUND;
	}

	struct stat statBuffer;

	// TODO: fix rare race condition:
	// between the reading and checking of stat, if the
	// file changes because of external effects, it might 
	// check an outdated value, leading to incorrect behaviour.

	// Aquire stat lock ?
	
	// Init file stat for further checks
	if(lstat(path,&statBuffer)==-1){
		switch(errno){ //TODO: Implement all possible lstat errors
			case ENOENT:
				_serve_error(socketFd,E_FILE_DNE);
				return E_FILE_DNE;
		}
	}

	// Check if file is non-regular (socket,symbolic link, block device, directory, character device, FIFO)
	if(!S_ISREG(statBuffer.st_mode)){
		_serve_error(socketFd,E_FILE_IS_NON_REGULAR);
		return E_FILE_IS_NON_REGULAR;
	}

	// Check if file is empty
	if(statBuffer.st_size==0){
		_serve_error(socketFd,E_FILE_IS_EMPTY);
		return E_FILE_IS_EMPTY;
	}

	// Check if file is too large
	if(statBuffer.st_size>ASTRAL_MAX_RESOURCE_SIZE){
		_serve_error(socketFd,E_FILE_TOO_LARGE);
		return E_FILE_TOO_LARGE;
	}

	// Check read access
	if(access(path,R_OK)!=F_OK){
		_serve_error(socketFd,E_FILE_READ_ACCESS_DENIED);
		return E_FILE_READ_ACCESS_DENIED;
	}
	
	//Read content, pass into fd.
	int fileFd = open(path,O_RDONLY);
	enforce(fileFd>=0,"Failed opening requested file.");
	int res = _serve_fd(socketFd,fileFd,ASTRAL_READ_CHUNK_SIZE,statBuffer.st_size);
	if(res != 0){
		_serve_error(socketFd,res);
		return res;
	}
	return 0; //Success
}

void _serve_error(int socketFd, astral_error_t error){
	fprintf(stderr,"Served standard error page. %s\n",astralErrorStr[error]);
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
	enforce(sc1==strnlen(headerBuf,512), "Failed writing to socket.");
	enforce(sc2==strnlen(errorPageFallback,512), "Failed writing to socket.");
	enforce(close(socketFd)==0,"Failed to close socket.");
}
// Reads file in chunks of bufferSize at a time and sends
// all data to socketFd. Closes socketFd and returns 0
// on success. Returns astral error code on error.
int _serve_fd(int const socketFd, int const fileFd, unsigned int const bufferSize,unsigned int const fileSize){
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

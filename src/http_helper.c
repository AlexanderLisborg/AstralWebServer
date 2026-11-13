
#include "astral_error.h"
#include "http_helper.h"

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>

// HTTP/1.0 syntax sourced from https://www.w3.org/Protocols/HTTP/1.0/spec.html#Request

int _serve_resource(int socketFd, char_span_t uri, pathmap_t * pathmap);
void _serve_error(int socketFd, astral_error_t error);
int _serve_fd(int const socketFd, int const fileFd, unsigned int const bufferSize,unsigned int const fileSize);
int _parse_header(http_header_args_t *header,char* request);

int _check_uri(char_span_t uri);
int _make_path_ALLOCATE(char** const pathALLOCATE, char_span_t uri,pathmap_t pathmap);
int _check_path(char * const path, unsigned int pathLength);
int _make_stat(struct stat * statBuffer, char * const path);
int _check_stat(struct stat * statBuffer);
int _check_access(char * const path);

// parses HTTP header, serves the requested resource (if it
// is mapped in pathmap, the mapped to file is served). 
// If an error occurs during this process, always serve a 
// standard error page response as to not expose sensitive 
// information to the requester.
int handle_http_request(int fd, char* request,pathmap_t * pathmap){
	http_header_args_t header = {0};
	_parse_header(&header,request);
	// if GET method with nonempty args, handle.
	if(header.method==GET && 
		header.resourceURI.head!=0 &&
		header.resourceURI.length!=0 &&
		header.versionMajor!=0 &&
		header.versionMinor!=0
	){
		int suc = _serve_resource(fd,header.resourceURI,pathmap);
		if(suc!=0){
			return suc;
		}
	}
}

// Parse http header, fill out the supplied instance 
// of the http_header_args struct accordingly. The values
// in the struct can hereafter be used to handle the request
// appropriately.
int _parse_header(http_header_args_t *header,char* request){
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
	return 0;
}

// Try to serve a requested resource. If the uri is mapped to
// a filepath in pathmap, try to serve that file instead. 
// Perform verious checks on the uri and derived path such that
// resources outside the root folder is not served and such that
// relevant errors are printed to the server admin when access fails.
int _serve_resource(int socketFd, char_span_t uri, pathmap_t * pathmap){
	int suc = _check_uri(uri);
	if(suc!=0){
		_serve_error(socketFd,suc);
		return suc;
	}

	// Construct path according to URI
	char* path;
	int pathLength = _make_path_ALLOCATE(&path, uri, *pathmap);
	
	// Read file info into statBuffer.
	// TODO: fix rare race condition:
	// between the reading and checking of stat, if the
	// file changes because of external effects, it might 
	// check an outdated value, leading to incorrect behaviour.
	struct stat statBuffer;
	suc = _make_stat(&statBuffer,path);
	if(suc!=0){
		_serve_error(socketFd,suc);
		return suc;
	}

	// Check stat such that the file is valid.
	suc = _check_stat(&statBuffer);
	if(suc!=0){
		_serve_error(socketFd,suc);
		return suc;
	}

	// Check access rights to file by running user.
	suc = _check_access(path);
	if(suc!=0){
		_serve_error(socketFd,suc);
		return suc;
	}

	// Read content, pass into fd.
	int fileFd = open(path,O_RDONLY);
	enforce(fileFd>=0,"Failed opening requested file.");
	suc = _serve_fd(socketFd,fileFd,ASTRAL_READ_CHUNK_SIZE,statBuffer.st_size);
	if(suc != 0){
		_serve_error(socketFd,suc);
		return suc;
	}
	return 0; //Success
}

// Serve standard error page.
void _serve_error(int socketFd, astral_error_t error){
	fprintf(stdout,"Served standard error page. Cause: %s\n",astralErrorStr[error]);
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

// Check uri length. On success, return 0. On failure, return
// astral error code.
int _check_uri(char_span_t uri){
	// Check uri length
	if(uri.length>ASTRAL_MAX_URI_LENGTH){
		return E_URI_TOO_LONG;
	}
	return 0;
}

// Construct path according to uri, allocate memory for path
// and set pathAllocate to beginning of allocation. If uri is 
// mapped to a filepath in pathmap, allocate and set that path
// instead. On success, return 0. On failure, return astral
// error code. 
int _make_path_ALLOCATE(char** const pathALLOCATE, char_span_t uri,pathmap_t pathmap){
	char* path=NULL;
	for(int i = 0 ; i < pathmap.size ; i++){
		if(strncmp(uri.head,pathmap.map[i].key,uri.length)==0){
			size_t valueLen = strnlen(pathmap.map[i].value,ASTRAL_MAX_URI_LENGTH);
			if(valueLen>=ASTRAL_MAX_URI_LENGTH){continue;}
			
			// Make space for adding . or ./ automatically.
			int pathLen=valueLen;
			if(*(pathmap.map[i].value)!='.'){
				if(*(pathmap.map[i].value)!='/'){
					pathLen++;
				}
				pathLen++;
			}

			path = (char*) malloc(sizeof(char)*pathLen);
			enforce(path!=NULL,"Failed malloc.");
			strncpy(path+(valueLen-pathLen),pathmap.map[i].value,valueLen);
			
			// Since all paths are relative, add . or ./ 
			// automatically if the mapped to path does 
			// not contain them
			if(pathLen-valueLen == 1){*path='.';}
			else if(pathLen-valueLen == 2){*path='.'; *(path+1)='/';}
			*pathALLOCATE = path;
			return pathLen;
		}
	}
	path = (char*) malloc(sizeof(char)*(uri.length+1));
	enforce(path!=NULL, "Failed malloc.");
	strncpy(path+1,uri.head,uri.length);
	*path = '.';
	*pathALLOCATE = path;
	return (uri.length+1);
}

// Check path for illegal escape sequence. On success, return 0.
// On failure, return astral error code.
int _check_path(char * const path, unsigned int pathLength){
	// Check illegal escape sequence ".." in path.
	if((memmem(path, pathLength, "..", 2)!=0 )){
		return E_URI_ESCAPE_SEQUENCE_FOUND;
	}
	return 0;
}

// Try to read file information into statBuffer. On success,
// return 0. On failure, return astral error code.
int _make_stat(struct stat * statBuffer, char * const path){
	// Init file stat for further checks
	if(lstat(path,statBuffer)==-1){
		switch(errno){ //TODO: Implement all possible lstat errors
			case ENOENT:
				return E_FILE_DNE;
		}
	}
	return 0;
}

// Check stat for illegal filetype, illegal file size. 
// On success, return 0. On failure, return astral error code.
int _check_stat(struct stat * statBuffer){
	// Check if file is non-regular (socket,symbolic link, block device, directory, character device, FIFO)
	if(!S_ISREG((*statBuffer).st_mode)){
		return E_FILE_IS_NON_REGULAR;
	}

	// Check if file is empty
	if((*statBuffer).st_size==0){
		return E_FILE_IS_EMPTY;
	}

	// Check if file is too large
	if((*statBuffer).st_size>ASTRAL_MAX_RESOURCE_SIZE){
		return E_FILE_TOO_LARGE;
	}

	return 0;
}
// Check user access of file. On success, return 0. On
// failure, return astral error code. 
int _check_access(char * const path){
	// Check read access
	if(access(path,R_OK)!=F_OK){
		return E_FILE_READ_ACCESS_DENIED;
	}
	return 0;
}
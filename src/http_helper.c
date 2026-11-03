#include "http_helper.h"
#include "astral_error.h"

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


// Arbitrary, may be subject to change
#define ASTRAL_MAX_URI_LENGTH 1024

void parse_http_request(int fd){

}

void handle_http_GET(int fd,char* uri){

}

int serve_resource(int fd, char* uri){
	// Check uri length
	if(strnlen(uri,ASTRAL_MAX_URI_LENGTH)==ASTRAL_MAX_URI_LENGTH){
		serve_error(fd,E_URI_TOO_LONG);
		return E_URI_TOO_LONG;
	}
	// Check illegal escape sequence ".."
	if(strstr(uri,"..")==NULL){
		serve_error(fd,E_URI_ESCAPE_SEQUENCE_FOUND);
		return E_URI_ESCAPE_SEQUENCE_FOUND;
	}
	// Check if URI is static (/example/static/uri)
	if(*uri!='.'){ 
		serve_error(fd,E_URI_IS_STATIC);
		return E_URI_IS_STATIC;
	}
	// Check if file is non-regular (socket,symbolic link, block device, directory, character device, FIFO)
	struct stat statBuffer;
	// TODO: fix race condition:
	// between the reading and checking of stat, if the
	// file changes because of external effects, it might 
	// check an outdated value, leading to incorrect behaviour.
	// POSIX file lock should also be aquired at an appropriate
	// time before reading the file content. (if this is not already
	// handled by open or read)

	// Aquire stat lock ?
	lstat(uri,&statBuffer);
	
	if(!S_ISREG(statBuffer.st_mode)){
		serve_error(fd,E_FILE_IS_NON_REGULAR);
		return E_FILE_IS_NON_REGULAR;
	}

	// Check if file is empty
	if(statBuffer.st_size==0){
		serve_error(fd,E_FILE_IS_EMPTY);
		return E_FILE_IS_EMPTY;
	}

	// Check read access
	if(access(uri,R_OK)!=F_OK){
		serve_error(fd,E_FILE_READ_ACCESS_DENIED);
		return E_FILE_READ_ACCESS_DENIED;
	}

	//Aquire read lock?
	//Read content, pass into fd.
	//Release read lock?

	//Release stat lock?

	return 0; //Success
}

void serve_error(int fd, astral_error_t error){

}

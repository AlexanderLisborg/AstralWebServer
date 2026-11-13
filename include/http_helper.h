#ifndef __http_helper__
#define __http_helper__

#include "span.h"

// Arbitrary macro values, may be subject to change
#define ASTRAL_MAX_URI_LENGTH 1024
#define ASTRAL_MAX_REQUEST_LENGTH 2048
#define ASTRAL_MAX_RESOURCE_SIZE 1073741824 // 1 GB
#define ASTRAL_READ_CHUNK_SIZE 2097152 // 2 MB

typedef struct pathmapping{
	char key[ASTRAL_MAX_URI_LENGTH];
	char value[ASTRAL_MAX_URI_LENGTH];
}pathmapping_t;

typedef struct pathmap{
	pathmapping_t* map;
	unsigned int size;
} pathmap_t;

typedef enum http_method{
    GET,QUERY,HEAD,POST,PUT,DELETE,CONNECT,OPTIONS,TRACE,PATCH
} http_method_t;

typedef struct http_header_args{
    http_method_t method;
    unsigned char versionMajor;
    unsigned char versionMinor;
    char_span_t resourceURI;
    
    
    // TODO: Implement according to https://httpwg.org/specs/rfc7230.html#request.line
    // A starting point is to implement the parsing of only 
    // GET /hello.txt HTTP/1.1 , disregardng any other arguments.

    // For a more complete implementation:
    // Investigate how the http request parser should be
    // implemented (can it be a simple DFA implementation
    // or does it require a grammar implementation).

    // A starting point is implementing parsing of the
    // information below (these were included in standard 
    // firefox get requests when testing):

    // host
    // user-agent
    // accept
    // accept-language
    // accept-encoding
    // connection
    // upgrade
    // upgrade-insecure-requests
    // sec-fetch-dest
    // sec-fetch-mode
    // sec-fetch-site
    // sec-fetch-user
    // priority
} http_header_args_t; 

// Parse http request and handle it appropriately.
int handle_http_request(int fd, char* request,pathmap_t * pathmap);

#endif // guard
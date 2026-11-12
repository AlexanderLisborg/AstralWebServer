#ifndef __http_helper__
#define __http_helper__

#include "span.h"

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
void handle_http_request(int fd, char* request);

#endif // guard
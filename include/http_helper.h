#ifndef __http_helper__
#define __http_helper__

typedef enum http_method{
    GET,QUERY,HEAD,POST,PUT,DELETE,CONNECT,OPTIONS,TRACE,PATCH
} http_method_t;

typedef struct http_header_args{
    http_method_t method;
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

    // http version
    // resource uri
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
void parse_http_request(int fd, char* request);


void handle_http_GET(int fd, char* uri);

// If uri is static or includes '..', send user to 
// 404 forbidden page. Check filetype, file size.
// And respond accordingly.
// Tip: Write http response to fd something like...
//sprintf(buf, ""
//	    "HTTP/1.0 200 OK\r\n"
//	    "Server: OSTEP WebServer\r\n"
//	    "Content-Length: fileSize\r\n"
//	    "Content-Type: fileType\r\n\r\n"
//);
// write(fd,buf,strlen(buf))
// Is there a faster way of doing this than sprintf?
// Basic approach would be to mmap the file into mem
// and write it to fd, does that always work? Is it 
// efficient? Since this is probably the largest work 
// done by the server, optimizing away unnessecary memory
// duplication may be relevant. What if file is read into
// fd directly instead of mapped to memory then written to
// fd. Is this possible? 
int serve_resource(int fd, char* uri);



#endif // guard
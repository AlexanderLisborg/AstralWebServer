#ifndef __http_helper__
#define __http_helper__

// Parse http request and handle it appropriately.
void parse_http_request(int fd);


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
void serve_resource(int fd, char* uri);

// If something goes wrong when serving content,
// respond with an error page which describes the
// error to users. (Not to develoeprs)
void serve_error(int fd, char errorNumber);



#endif // guard
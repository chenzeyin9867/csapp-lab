#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";
void doit(int fd);
void read_requesthdrs(rio_t *rp); 
int parse_uri(char *uri, char *host, char *port, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) ;
void generate_request_header(char* request_header, char *filename, char *host, char *port);

int main(int argc, char** argv)
{
    
    printf("%s", user_agent_hdr);
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    listenfd = Open_listenfd(argv[1]); // Proxy listen the fd, waiting for other clients to connect
    while (1) {
	    clientlen = sizeof(clientaddr);
	    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
	    doit(connfd);                                             //line:netp:tiny:doit
	    Close(connfd);                                            //line:netp:tiny:close
    }
    
    return 0;
}

/*
 * doit - handle one HTTP request/response transaction, send the request to corresponding server, then reply the answer
 */
/* $begin doit */
void doit(int fd) 
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    char host[MAXLINE], port[10];
    memset(host, 0, MAXLINE);
    memset(filename, 0, MAXLINE);
    memset(port, 0, MAXLINE);
    rio_t rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;
    printf("buf is %s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }                                                    //line:netp:doit:endrequesterr
    
    read_requesthdrs(&rio);                              //line:netp:doit:readrequesthdrs

    /* Parse URI from GET request */
    if(parse_uri(uri, host, port, filename, cgiargs) < 0){
        perror('parse_uri');
        return;
    }       
                                                 
    /* Generate the Request-Header from proxy to end server */
    /* Header consists of Filed of Host\User-Agent\Connection\Proxy-Connection */
    char request_header[MAXLINE];
    generate_request_header(request_header, filename, host, port);
    printf("filename:%s\thost:%s\tport:%s\n", filename, host, port);
    /* Connect to the corresponding server, wait for reply */
    int clientfd = Open_clientfd(host, port);

    Rio_writen(clientfd, request_header, strlen(request_header));
    // Rio_readlineb(&rio, buf, MAXLINE);
    Rio_readinitb(&rio, clientfd);
    /* send back the info to origin client */
    printf("Start recv reply from server\n");
    int n = 0;
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){	    
	    Rio_writen(fd, buf, n);
    }
        
    // printf("ret from server:\n%s\n", buf); 
    printf("End this Do loop\n");
    Close(clientfd);
    return;    
}


/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */


/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *host, char *port, char *filename, char *cgiargs) 
{
    char *ptr;
    // example url is GET http://www.cmu.edu:80/hub/index.html HTTP/1.1
    // transform into wanted format
    char m_protocal[8];
    printf("before parsem uri:%s\thost:%s\tport:%s\tfilename:%s\t\n", uri, host, port, filename);
    // char m_port[10];
    strncpy(m_protocal, uri, 7);
    if(strcmp(m_protocal, "http://")){
        perror("http format faltal");
        return -1; // faltal
    }

    int p = 7;
    while(uri[p] != '/' && uri[p] != ':'){
        *(host++) = uri[p];
        ++p;
    } 

    if(uri[p] == ':'){
        ++p;
        // parse the port num
        while(uri[p] != '/'){
            *(port++) = uri[p];
            ++p;
        }
    }else{
        strcpy(port, "80"); // default port num
    }
    // begin with '/'
    while(p < strlen(uri)){
        *(filename++) = uri[p++];
    }

    // printf("host:\t%s\nport:\t%s\nfilename:\t%s\n", host, port, filename);
    return 1;
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
    
    // return;
}
/* $end clienterror */


/**
 *  Generate the request header
 */
void generate_request_header(char* request_header, char *filename, char *host, char *port){
    strcpy(request_header, "GET ");
    strcat(request_header, filename);
    strcat(request_header, " HTTP/1.0\r\n");
    strcat(request_header, "Host: ");
    strcat(request_header, host);
    strcat(request_header, ":");
    strcat(request_header, port);
    strcat(request_header, "\r\n");
    strcat(request_header, user_agent_hdr);
    strcat(request_header, conn_hdr);
    strcat(request_header, prox_hdr);
    strcat(request_header, "\r\n");
    printf("Request send from Proxy:\n%s", request_header);
    return;
}
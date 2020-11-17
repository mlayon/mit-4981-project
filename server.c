/* 
HTTP server that serves static and
 *          dynamic content with the GET method.
 *          usage: server port num
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFSIZE 1024
#define MAXERRS 16

void *acceptRequest(void *vargp);

void getErrorCheck(FILE *stream, char m[]);
void parse_url(char filename[], char uri[], char cgiargs[]);
void display_content(FILE *stream, int fd, char *p, char filename[], char filetype[], struct stat sbuf);
/*
 * error - wrapper for perror used for bad syscalls
 */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

/*
 * cerror - returns an error message to the client
 */
void cerror(FILE *stream, char *cause, char *errno,
            char *shortmsg, char *longmsg)
{
    fprintf(stream, "HTTP/1.1 %s %s\n", errno, shortmsg);
    fprintf(stream, "Content-type: text/html\n");
    fprintf(stream, "\n");
    fprintf(stream, "<html><title>Error</title>");
    fprintf(stream, "<body bgcolor="
                    "ffffff"
                    ">\n");
    fprintf(stream, "%s: %s\n", errno, shortmsg);
    fprintf(stream, "<p>%s: %s\n", longmsg, cause);
}

int main(int argc, char **argv)
{
    /* variables for connection management */
    int parentfd; /* parent socket */

    int optval;                    /* flag value for setsockopt */
    struct sockaddr_in serveraddr; /* server address */

    /* variables for connection I/O */
    // FILE *stream;           /* stream version of childfd */

    int portno; /* port */

    // command lines
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    /* open socket descriptor */
    parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (parentfd < 0)
        error("ERROR opening socket");

    /* allows us to restart server immediately */
    optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    /* bind port to socket */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);
    if (bind(parentfd, (struct sockaddr *)&serveraddr,
             sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    // acceptRequest(clientaddr, parentfd);
    pthread_t thread_id;
    int i = 0;

    printf("Before Thread\n");
    while (i < 10)
    {
        fprintf(stderr, "Number of times: %d", i);
        pthread_create(&thread_id, NULL, acceptRequest, &parentfd);
        pthread_join(thread_id, NULL);
        
        i++;
    }

    

    close(parentfd);
    return 0;
}

// void *myThreadFun(void *vargp)
void *acceptRequest(void *vargp)
{

    struct sockaddr_in clientaddr; /* client address */
    int clientlen;                 /* byte size of client's address */
    int childfd;                   /* child socket */
    struct hostent *hostp;         /* client host info */
    FILE *stream;                  /* stream version of childfd */
    char *hostaddrp;               /* dotted decimal host addr string */
    char buf[BUFSIZE];             /* message buffer */

    char method[BUFSIZE];  /* request method get, head, post*/
    char uri[BUFSIZE];     /* request url */
    char version[BUFSIZE]; /* request method */

    char filename[BUFSIZE]; /* path derived from url */
    char filetype[BUFSIZE]; /* path derived from url */
    char cgiargs[BUFSIZE];  /* cgi argument list */
    char *p;                /* temporary pointer */
    int is_static;          /* static request? */
    struct stat sbuf;       /* file status */
    int fd;                 /* static content filedes */
    int pid;                /* process id from fork */
    int wait_status;        /* status from wait */

    int *parentfd = (int *)vargp;

    /* getting ready to accept connection requests */
    if (listen(*parentfd, 9) < 0) /* allow 5 requests to queue up */
        error("ERROR on listen");
    /* 
   * main loop: wait for a connection request, parse HTTP,
   * serve requested content, close connection.
   */
    clientlen = sizeof(clientaddr);

    /* wait for a connection request */
    childfd = accept(*parentfd, (struct sockaddr *)&clientaddr, &clientlen);
    if (childfd < 0)
        error("ERROR on accept");

    /* determine who sent the message */
    //     hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
    //                           sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    hostp = gethostbyname("127.0.0.1");
    if (hostp == NULL)
        error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
        error("ERROR on inet_ntoa\n");

    /* open the child socket descriptor as a stream */
    if ((stream = fdopen(childfd, "r+")) == NULL)
        error("ERROR on fdopen");

    /* get the HTTP request line */
    fgets(buf, BUFSIZE, stream);
    //testing head
    //   char bufHead[25] = "HEAD /index.html HTTP/1.1";
    //  memcpy(buf,bufHead, strlen(bufHead));
    printf("Request line!%s", buf);
    sscanf(buf, "%s %s %s\n", method, uri, version);

    //compare GET METHOD
    printf("COMPARE%s vs %i,", method, strcasecmp(method, "GET")); // 0 means they're requivalent
    if (strcasecmp(method, "GET"))
    {
        printf("Yoo");
        //  getErrorCheck(stream, method);
        close(childfd);
        pthread_exit(NULL);
    }

    /* read (and ignore) the HTTP headers */
    fgets(buf, BUFSIZE, stream);
    while (strcmp(buf, "\r\n"))
    {
        fgets(buf, BUFSIZE, stream);
        printf("%s", buf);
    }

    /* parse the url (/filename.html)] */
    if (!strstr(uri, "cgi-bin"))
    { /* static content */
        is_static = 1;

        parse_url(filename, uri, cgiargs);
    }

    /* make sure the file exists */
    if (stat(filename, &sbuf) < 0)
    {
        cerror(stream, filename, "404", "Not found",
               "Could not find file.");
        fclose(stream);
        close(childfd);
        pthread_exit(NULL);
    }

    /* Display content */
    if (is_static)
    {
        display_content(stream, fd, p, filename, filetype, sbuf);
    }

    /* clean up */
    fclose(stream);
    close(childfd);

    pthread_exit(NULL);
}

void getErrorCheck(FILE *stream, char m[])
{
    cerror(stream, m, "501", "Not Implemented",
           "Does not implement this method");
    fclose(stream);
}

void display_content(FILE *stream, int fd, char *p, char filename[], char filetype[], struct stat sbuf)

{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");

    else
        strcpy(filetype, "text/plain");

    /* print response header */
    fprintf(stream, "HTTP/1.1 200 OK\n");
    fprintf(stream, "Content-length: %d\n", (int)sbuf.st_size);
    fprintf(stream, "Content-type: %s\n", filetype);
    fprintf(stream, "\r\n");
    fflush(stream);

    /* Use mmap to return arbitrary-sized response body */
    fd = open(filename, O_RDONLY);
    p = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    fwrite(p, 1, sbuf.st_size, stream);
    munmap(p, sbuf.st_size);
}

void parse_url(char filename[], char uri[], char cgiargs[])
{
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    printf("Uri: %s", uri); // this is the url
    if (uri[strlen(uri) - 1] == '/')
        strcat(filename, "index.html");
}
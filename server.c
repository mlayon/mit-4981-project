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
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
// #include "threadpool.h"

#define BUFSIZE 1024
#define MAXERRS 16
#define THREAD_POOL_SIZE 10
struct node
{
    struct node *next;
    int *client_socket;
};
typedef struct node node_t;

node_t *head = NULL;
node_t *tail = NULL;
pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
void *handle_connection(void *p_client_socket);
void *thread_function(void *arg);
void display_content(FILE *stream, int fd, char *p, char filename[], char filetype[], struct stat sbuf);
void parse_url(char filename[], char uri[], char cgiargs[]);
void enqueue(int *client_socket)
{
    node_t *newnode = malloc(sizeof(node_t));
    newnode->client_socket = client_socket;
    newnode->next = NULL;
    if (tail == NULL)
    {
        head = newnode;
    }
    else
    {
        tail->next = newnode;
    }
    tail = newnode;
}

int *dequeue()
{
    if (head == NULL)
    {
        return NULL;
    }
    else
    {
        int *result = head->client_socket;
        node_t *temp = head;
        head = head->next;
        if (head == NULL)
        {
            tail = NULL;
        }
        free(temp);
        return result;
    }
}

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

void *thread_function(void *arg)
{
    while (true)
    {

        int *pclient;
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condition_var, &mutex);
        pclient = dequeue();
        pthread_mutex_unlock(&mutex);

        if (pclient != NULL)
        {
            //we have a connection
            handle_connection(pclient);
        }
    }
}

void *handle_connection(void *p_client_socket)
{

    int childfd = *((int *)p_client_socket);
    free(p_client_socket);
    /* variables for connection I/O */
    FILE *stream = NULL;    /* stream version of childfd */
    char buf[BUFSIZE];      /* message buffer */
    char method[BUFSIZE];   /* request method get, head, post*/
    char uri[BUFSIZE];      /* request url */
    char version[BUFSIZE];  /* request method */
    char filename[BUFSIZE]; /* path derived from url */
    char filetype[BUFSIZE]; /* path derived from url */
    char cgiargs[BUFSIZE];  /* cgi argument list */
    char *p = NULL;         /* temporary pointer */
    int is_static;          /* static request? */
    struct stat sbuf;       /* file status */
    int fd;                 /* static content filedes */
    int pid;                /* process id from fork */
    int wait_status;        /* status from wait */

    /* open the child socket descriptor as a stream */
    if ((stream = fdopen(childfd, "r+")) == NULL)
    {
        error("ERROR on fdopen");
    }

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
        // continue;
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

        close(childfd);
        fclose(stream);
        return NULL;
        // continue;
    }

    /* Display content */
    if (is_static)
    {
        display_content(stream, fd, p, filename, filetype, sbuf);
    }

    /* clean up */
    close(childfd);
    fclose(stream);

    fprintf(stderr, "closing connection\n");
    return NULL;
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

int main(int argc, char **argv)
{

    /* variables for connection management */
    int parentfd;                  /* parent socket */
    intptr_t childfd;              /* child socket */
    int portno;                    /* port */
    int optval;                    /* flag value for setsockopt */
    int clientlen;                 /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server address */
    struct sockaddr_in clientaddr; /* client address */
    struct hostent *hostp = NULL;  /* client host info */
    char *hostaddrp = NULL;        /* dotted decimal host addr string */

    // command lines
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);
    //first off create a bunch of threads to handle future connections.
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    // threadpool *tPool = NULL;
    // tPool = create_threadpool(THREAD_POOL_SIZE);
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

    /* getting ready to accept connection requests */
    if (listen(parentfd, 1) < 0) /* allow 100 requests to queue up */
        error("ERROR on listen");
    clientlen = sizeof(clientaddr);
    /* 
   * main loop: wait for a connection request, parse HTTP,
   * serve requested content, close connection.
   */
    int i = 0;
    while (true)
    {

        /* wait for a connection request */
        childfd = accept(parentfd, (struct sockaddr *)&clientaddr, &clientlen);
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

        // dispatch(tPool, (dispatch_fn)handle_connection, (void *)childfd);
        fprintf(stderr, "%d%s", i, " this is a test");
        i++;

        int *pclient = malloc(sizeof(int));
        *pclient = childfd;

        //make sure only one thread messes with the queue at a time
        pthread_mutex_lock(&mutex);
        enqueue(pclient);
        pthread_cond_signal(&condition_var);
        pthread_mutex_unlock(&mutex);
    }
    return 0;
}
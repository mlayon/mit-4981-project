/* 
HTTP server that serves static and
 *          dynamic content with the GET method.
 *          usage: ./server 
 *              or ./server --gui 
 *              or ./server -p <portno>
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
#include <stdint.h>
#include <stdbool.h>
// #include "pthreadpool.h"
#include "config.h"
#include "queue.h"
#include "helper.h"

#define BUFSIZE 1024
#define MAXERRS 16

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

//CONFIG INFORMATION
char *errorfile;
//char *html_root = "./docs/docs2";
//default root
char *html_root;
void *handle_connection(void *p_client_socket);
void *thread_function();
void start_gui(void);

int main(int argc, char **argv)
{    
    /* variables for connection management */
    int parentfd;                  /* parent socket */
    int childfd;                   /* child socket */
    int portno = 0;                    /* port */
    socklen_t clientlen;                 /* byte size of client's address */
    struct hostent *hostp;         /* client host info */
    char *hostaddrp;               /* dotted decimal host addr string */
    int optval;                    /* flag value for setsockopt */
    struct sockaddr_in serveraddr = {0}; /* server address */
    struct sockaddr_in clientaddr = {0}; /* client address */
    Config conf;
    int parse_status;
    char subprocess;
    int pid;                /* process id from fork */

    // command lines
    if (argc == 2) {
        // If appropriate flag is given, start gui
        if (strcmp(argv[1], "-gui") == 0) {
            start_gui();
        }

    // for ./server -p <portno>
    } else if (argc == 3) {
        if (strcmp(argv[1], "-p") == 0) {
            portno = atoi(argv[2]);
        }
    } // can't have more than 3 args
     else if (argc > 3) {
        fprintf(stderr, "usage: %s or %s -gui or %s -p <portnumber>\n", argv[0], argv[0], argv[0]);
        exit(1);
    }

    // Parse through config file
    if((parse_status = get_config_file(&conf)) == 0) {
        perror("ERROR while parsing configuration file");
        return 0;

    } 

    // if port no is not assigned yet, use the one from config file
	if (portno == 0) { 
		portno = conf.port;
	} 

    subprocess = conf.subprocess;
    html_root = conf.root;
    errorfile = conf.error;
    
    
    //first off create a bunch of threads to handle future connections.
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }
    /* open socket descriptor */
    parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (parentfd < 0)
        error("ERROR opening socket");

    /* allows us to restart server immediately */
    optval = 1;
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    /* bind port to socket */
    bind_port(parentfd, serveraddr, portno);

    /* getting ready to accept connection requests */
    if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */
        error("ERROR on listen");

    /* 
   * main loop: wait for a connection request, parse HTTP,
   * serve requested content, close connection.
   */
    clientlen = sizeof(clientaddr);
    
    int i = 0;
    // if we're using threads
    if (subprocess == 't') {
        while (true)
        {
            //parent process waiting to accept a new connection
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

            i++;

            int *pclient = malloc(sizeof(int));
            *pclient = childfd;

            //make sure only one thread messes with the queue at a time
            pthread_mutex_lock(&mutex);
            enqueue(pclient);
            pthread_cond_signal(&condition_var);
            pthread_mutex_unlock(&mutex);
        }
    } else if (subprocess == 'p') {
        while (true)
        {
            //parent process waiting to accept a new connection
            /* wait for a connection request */
            childfd = accept(parentfd, (struct sockaddr *)&clientaddr, &clientlen);
            if (childfd < 0)
                error("ERROR on accept");
            
            pid = fork();
            // handle connection if we're on the child process
            if (pid == 0) {

                while (true) {
                    hostp = gethostbyname("127.0.0.1");
                    if (hostp == NULL)
                        error("ERROR on gethostbyaddr");

                    hostaddrp = inet_ntoa(clientaddr.sin_addr);
                    if (hostaddrp == NULL)
                        error("ERROR on inet_ntoa\n");

                    handle_connection(&childfd);
                    break;
                }

            } else {
                close(childfd);
                continue;
            }
        }
    }

    return 0;
}

void *thread_function()
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

    /* variables for connection I/O */
    FILE *stream;           /* stream version of childfd */
    char buf[BUFSIZE];      /* message buffer */
    char method[BUFSIZE];   /* request method get, head, post*/
    char uri[BUFSIZE];      /* request url */
    char version[BUFSIZE];  /* request method */
    char filename[BUFSIZE]; /* path derived from url */
    char filetype[BUFSIZE]; /* path derived from url */
    char cgiargs[BUFSIZE];  /* cgi argument list */
    char *p = NULL;                /* temporary pointer */
    int is_static;          /* static request? */
    struct stat sbuf;       /* file status */
    int fd = 0;                 /* static content filedes */
    int choice = 0;
    int childfd = *((int *)p_client_socket);
    char c1[BUFSIZE];  
    char content_len2[BUFSIZE];  
    char content_len4[BUFSIZE];  

    /* open the child socket descriptor as a stream */
    if ((stream = fdopen(childfd, "r+")) == NULL)
        error("ERROR on fdopen");

    /* get the HTTP request line */
    fgets(buf, BUFSIZE, stream);

    //Request line head! "GET /filename.html";
    printf("%s", buf);

    sscanf(buf, "%s %s %s\n", method, uri, version);

   
 //compare
    // 0 means they're requivalent
    if (strcmp(method, "GET") == 0)
    {
        choice = 0;
    }
    else if (strcmp(method, "HEAD") == 0)
    {
        choice = 1;
    }
    else if (strcmp(method, "POST") == 0)
    {
        choice = 2;
    }
    else
    {
        get_error_check(childfd, stream, errorfile);
        close(childfd);
        fprintf(stderr, "closing connection\n");
        return NULL;
    }

    /* read (and ignore) the HTTP headers */
    fgets(buf, BUFSIZE, stream);
    int count = 0;
    while (strcmp(buf, "\r\n"))
    { 
        fgets(buf, BUFSIZE, stream);
        printf("%s", buf);
          if(count == 2){
         sscanf(buf, "%s %s\n", c1, content_len2);
          }
          count++;
           sscanf(buf, "%s %s\n", c1, content_len4);
    }


    /* parse the url (/filename.html)] */
    if (!strstr(uri, "cgi-bin"))
    { /* static content */
        is_static = 1;

        parse_url(filename, uri, cgiargs, html_root);

    }

    /* make sure the file exists */
    if (stat(filename, &sbuf) < 0)
    {
        cerror(childfd, stream, errorfile);
        fclose(stream);
        close(childfd);
        fprintf(stderr, "\n*****closing connection*****\n");
        return NULL;
    }

    /* Display content */
    if (is_static && choice == 0)
    {
        printf("\nGet method\n");
        display_content(childfd, stream, fd, p, filename, filetype, sbuf);
    }
    else if(choice ==1)
    {
        printf("\nHead method: \n");

        if (strstr(filename, ".html"))
            strcpy(filetype, "text/html");

        else
            strcpy(filetype, "text/plain");

        /* print response header */
        fprintf(stream, "HTTP/1.1 200 OK\n");
        fprintf(stream, "Content-type: %s\n", filetype);
        fprintf(stream, "Content-length: %d\n", (int)sbuf.st_size);
        fprintf(stream, "\r\n");
        fflush(stream);
    }else{

        int len = atoi(content_len2) + 1; // curl

        if(len == 1){ // wget
           len = atoi(content_len4);
        }
       // printf("\nNum %d", len);
        fgets(buf,len, stream);
        printf("POSTed data %s", buf);
        printf("\nPOST method: \n");

        /* print response header */
        fprintf(stream, "HTTP/1.1 200 OK\n");
        fprintf(stream, "Content-length: %d\n", (int)sbuf.st_size );
        fprintf(stream, "Content-type: %s\n", filetype);
        fprintf(stream, "\r\n");
        fflush(stream);
        char * str = buf;
        send(childfd, str, strlen(str), 0);
    }
    /* clean up */
    fclose(stream);
    close(childfd);
    fprintf(stderr, "\n*****closing connection*****\n");
    return NULL;
}

// Creates child process and runs the gui program over it
void start_gui(void) {
    /*Spawn a child to run the program.*/
    pid_t pid=fork();
    if (pid==0) { /* child process */
        execv("gui", NULL);
        exit(127); /* only if execv fails */
    }
    else { /* pid!=0; parent process */
        waitpid(pid,0,0); /* wait for child to exit */
    }
    
}

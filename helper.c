#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
/*
 * error - wrapper for perror used for bad syscalls
 */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

/**
 * Get the size of a file.
 * @return The filesize, or 0 if the file does not exist.
 */
size_t get_file_size(const char *filename)
{
    struct stat st;
    if (stat(filename, &st) != 0)
    {
        return 0;
    }
    return st.st_size;
}
/*
 * cerror - returns an error message to the client
 */
void cerror(int childfd, FILE *stream, char *errorfile)
{

    int size404 = get_file_size(errorfile);
    /* print response header */
    fprintf(stream, "HTTP/1.1 200 OK\n");
    fprintf(stream, "Content-length: %d\n", size404);
    fprintf(stream, "Content-type: text/html\n");
    fprintf(stream, "\r\n");
    fflush(stream);

    FILE *f = fopen(errorfile, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, f);
    fclose(f);

    send(childfd, string, size404, 0);
   
}


// Display the contents of a given file
void display_content(int childfd, FILE *stream, int fd, char *p, char filename[], char filetype[], struct stat sbuf)

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
  
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, f);
    fclose(f);

    send(childfd, string, sbuf.st_size, 0);
    munmap(p, sbuf.st_size);
}

// Parse through the given URL
void parse_url(char filename[], char uri[], char cgiargs[], char *html_root)
{
    strcpy(cgiargs, "");
    strcpy(filename, html_root);
    strcat(filename, uri);

    if (uri[strlen(uri) - 1] == '/')
        strcat(filename, "index.html");
}

// Initialize the server socket address configurations  
void bind_port(int parentfd, struct sockaddr_in serveraddr, int portno)
{

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);
    if (bind(parentfd, (struct sockaddr *)&serveraddr,
             sizeof(serveraddr)) < 0)
        error("ERROR on binding");
}

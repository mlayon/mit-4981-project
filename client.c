
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BACKLOG 5
#define BUF_SIZE 100

#define PORT 49157

int main(int argc, char *argv[])
{
    struct hostent *hostinfo;
    struct sockaddr_in addr;
    int fd;
    ssize_t num_read;
    char buf[BUF_SIZE];
    
    // hostinfo = dc_gethostbyname("127.0.0.1");
    hostinfo = gethostbyname("127.0.0.1");
    if (hostinfo == NULL) {
        perror("cannot get host by name");
        return 0;
    }
    // fd = dc_socket(AF_INET, SOCK_STREAM, 0);
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("cannot create socket");
        return 0;
    }
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr = *(struct in_addr *) hostinfo->h_addr;
    // dc_connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    
    while((num_read = read(STDIN_FILENO, buf, BUF_SIZE)) > 0)
    {
        write(fd, buf, num_read);
    }
    
    close(fd);

    return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BACKLOG 5
#define BUF_SIZE 100

// https://www.sciencedirect.com/topics/computer-science/registered-port#:~:text=Ports%200%20through%201023%20are,be%20used%20dynamically%20by%20applications.
// /etc/services
#define PORT 49157

int main(int argc, const char * argv[])
{
    struct sockaddr_in addr;
    int sfd;
    
    // sfd = dc_socket(AF_INET, SOCK_STREAM, 0);
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("cannot create socket");
        return 0;
    } 

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // dc_bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return 0;
    }

    // dc_listen(sfd, BACKLOG);
    if (listen(sfd, BACKLOG) < 0) {
        perror("error in listen");
        exit(EXIT_FAILURE);
    }
    
    for(;;)
    {
        int cfd; // child fd
        ssize_t num_read;
        char buf[BUF_SIZE];

        // cfd = dc_accept(sfd, NULL, NULL);
        cfd = accept(sfd, NULL, NULL);
        // if ((cfd = accept(sfd, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) < 0) {
        //     perror("In accept");
        //     exit(EXIT_FAILURE);
        // }

        while((num_read = read(cfd, buf, BUF_SIZE)) > 0)
        {
            write(STDOUT_FILENO, buf, num_read);
        }
        
        close(cfd);
    }
    
    // dc_close(sfd); <- never reached because for(;;) never ends.

    return EXIT_SUCCESS;
}


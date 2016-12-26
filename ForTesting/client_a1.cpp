#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#define BUFFER_SIZE (1024)
#define IP_ADDRESS "localhost"
#define PORT 4444

int main(int argc, char *argv[])
{    
    unsigned short port = 0;
    char *address = NULL;
    
    int socket_fd = 0;
    struct sockaddr_in addr;
    struct hostent *host_info = NULL;
    char **iterator = 0;

    void *buf = malloc(BUFFER_SIZE + 1);
    char *message = NULL;

    address = strdup(IP_ADDRESS);
    port = PORT;
    host_info = gethostbyname(address);   

    bzero(&addr, sizeof(struct sockaddr_in));   
    addr.sin_family = PF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, host_info->h_addr, host_info->h_length);
    
    socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    connect(socket_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));

    message = (char*) buf;    
    message = strdup("manoonam");
    fprintf(stderr, "Before while\n");
    while (1)
    {
        if (0 >= send(socket_fd, buf, BUFFER_SIZE, 0)) 
            perror("send");                                   
    }
    fprintf(stderr, "After while\n");
    
    close(socket_fd);
    free(buf);
    free(address);    
    return EXIT_SUCCESS;
}

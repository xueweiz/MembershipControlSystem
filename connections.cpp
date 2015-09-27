#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "connections.h"

int bindSocket(int port)
{
    struct sockaddr_in svrAdd, clntAdd;
    
    //create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd < 0)
    {
        std::cout << "Cannot open socket" << std::endl;
        exit(1);
    }
    
    memset((char*) &svrAdd,0, sizeof(svrAdd));
    
    svrAdd.sin_family = AF_INET;
    svrAdd.sin_addr.s_addr = INADDR_ANY;
    svrAdd.sin_port = htons(port);
    
    //bind socket
    if(bind(sockfd, (struct sockaddr *)&svrAdd, sizeof(svrAdd)) < 0)
    {
        std::cout << "open_socket: Cannot bind" << std::endl;
        exit(1);
    }
    printf("Listening at port %d...\n", port);

    return sockfd;
}

void receiveUDP(int sockfd)
{
    int byte_count;
    socklen_t fromlen;
    struct sockaddr addr;
    char buf[5];

    fromlen = sizeof addr;
    byte_count = recvfrom(sockfd, buf, sizeof buf, 0, &addr, &fromlen);

    struct sockaddr_in *sin = (struct sockaddr_in *) &addr;

    printf("Received from: %s\n", inet_ntoa(sin->sin_addr));

    for (int i = 0; i < byte_count; ++i)
    {
        printf("%c - ", buf[i]);
    }
    printf("\n");
}

void sendUDP(int sockfd, int port)
{
    struct sockaddr_in servaddr,cliaddr;
    char sendline[1000] = {'a', 'b', 'c', 'd'};

    struct hostent *server;

    server = gethostbyname("localhost");
    //char *ip = inet_ntoa( ( (struct sockaddr_in*) &addr)->sin_addr);

    printf("Send to : %s\n", server->h_addr);

    if(server == NULL)
    {
        std::cout << "Host does not exist" << std::endl;
        exit(1);
    }

    memset((char *) &servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    memcpy((char *) &servaddr.sin_addr.s_addr,(char *) server -> h_addr, server -> h_length);
    servaddr.sin_port = htons(port);

    int ret = sendto(sockfd,sendline, 4, 0, (struct sockaddr *)&servaddr,sizeof(servaddr));

    if (ret == -1)
    {
        printf("Error in sendUDP: cannot send\n");
    }

    printf("Message sent\n");
}


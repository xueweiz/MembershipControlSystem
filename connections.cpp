#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "connections.h"

int bindSocket(int port)
{
    struct addrinfo hints, *res;
    int sockfd;
    int byte_count;
    socklen_t fromlen;
    struct sockaddr addr;
    char buf[512];
    char ipstr[INET6_ADDRSTRLEN];

    // get host info, make socket, bind it to port 4950
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, "4950", &hints, &res);
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    bind(sockfd, res->ai_addr, res->ai_addrlen);

    printf("Socket bind done\n");

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

    for (int i = 0; i < sizeof buf; ++i)
    {
        printf("%c - ", buf[i]);
    }
    printf("\n");
}

void sendUDP(int port)
{
    int sockfd;
    struct sockaddr_in servaddr,cliaddr;
    char sendline[1000] = {'a', 'b', 'c', 'd'};
    char recvline[1000];

    sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&servaddr,sizeof(servaddr));
    //servaddr.sin_family = AF_INET;
    //servaddr.sin_addr.s_addr=gethostbyname("localhost");
    //servaddr.sin_port=htons(port);

    struct hostent *server;

    server = gethostbyname("localhost");

    if(server == NULL)
    {
        std::cout << "Host does not exist" << std::endl;
        exit(1);
    }

    memset((char *) &servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    memcpy((char *) &servaddr.sin_addr.s_addr,(char *) server -> h_addr, server -> h_length);
    servaddr.sin_port = htons(port);

    sendto(sockfd,sendline, 4, 0, (struct sockaddr *)&servaddr,sizeof(servaddr));

    printf("Message sent\n");
}


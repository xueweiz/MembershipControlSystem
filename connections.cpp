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

int receiveUDP(int sockfd, char* buf, uint32_t len, std::string& sender)
{
    struct sockaddr addr;
    socklen_t fromlen = sizeof addr;

    int byte_count = recvfrom(sockfd, buf, len, 0, &addr, &fromlen);

    if (byte_count == -1)
    {
        printf("ERROR RECEIVING!!!\n");
        exit(-1);
    }

    struct sockaddr_in *sin = (struct sockaddr_in *) &addr;

    sender = inet_ntoa(sin->sin_addr);

    return byte_count;
}

void sendUDP(int sockfd, std::string& add, int port, char* buf, uint32_t len)
{
    struct sockaddr_in servaddr,cliaddr;
    struct hostent *server;

    server = gethostbyname(add.c_str());

    if(server == NULL)
    {
        std::cout << "Host does not exist" << std::endl;
        exit(1);
    }

    memset((char *) &servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    memcpy((char *) &servaddr.sin_addr.s_addr,(char *) server -> h_addr, server -> h_length);
    servaddr.sin_port = htons(port);

    int ret = sendto(sockfd,buf, len, 0, (struct sockaddr *)&servaddr,sizeof(servaddr));

    if (ret == -1)
    {
        printf("Error in sendUDP: cannot send\n");
    }
    //printf("Message sent\n");
}


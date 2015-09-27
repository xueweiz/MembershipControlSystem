/* Spreading part */

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "spread.h"
#include "connections.h"

int spreadFailure(int sockfd, int port, int machineId)
{
	std::string add = "localhost";
    char sendline[1000] = {'a', 'b', 'c', 'd'};

    sendUDP(sockfd, add, port, sendline, 4);
    sendUDP(sockfd, add, port, sendline, 4);
    sendUDP(sockfd, add, port, sendline, 4);
    sendUDP(sockfd, add, port, sendline, 4);
}

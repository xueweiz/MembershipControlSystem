/* Spreading part */

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "spread.h"
#include "connections.h"
#include "constant.h"

int spreadFailure(int sockfd, int port, int machineId)
{
	std::string add = "localhost";
    char sendline[1000] = {'a', 'b', 'c', 'd'};

    struct Message msg;
    msg.type = MSG_FAIL;

    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
}

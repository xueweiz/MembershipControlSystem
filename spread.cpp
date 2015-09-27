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

void spreadFailure(int sockfd, int port, int machineId)
{
	std::string add = "localhost";

    struct Message msg;
    msg.type = MSG_FAIL;
    msg.TTL = 3;

    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
}

void spreadLeave(int sockfd, int port, int machineId)
{
	std::string add = "localhost";

    struct Message msg;
    msg.type = MSG_LEAVE;
    msg.TTL = 3;

    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
    sendUDP(sockfd, add, port, (char*)&msg, sizeof(msg));
}

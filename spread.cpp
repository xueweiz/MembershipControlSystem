/* Spreading part */

#include <sstream>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "spread.h"
#include "connections.h"
#include "constant.h"

void ipString2Char4(std::string ip, char* buf) // buf must be size 4
{
    ip.replace(ip.find("."),1," ");
    ip.replace(ip.find("."),1," ");
    ip.replace(ip.find("."),1," ");
    
    std::stringstream ssip(ip);
    int a;
    ssip >> a; buf[0] = (char)a; 
    ssip >> a; buf[1] = (char)a; 
    ssip >> a; buf[2] = (char)a; 
    ssip >> a; buf[3] = (char)a; 
}

void spreadFailure(int sockfd, std::string dest, int port, std::string carrier)
{
    struct Message msg;
    msg.type = MSG_FAIL;
    msg.TTL = 3;

    ipString2Char4(carrier, msg.carrierAdd);

    sendUDP(sockfd, dest, port, (char*)&msg, sizeof(msg));
}

void spreadLeave(int sockfd, std::string dest, int port, std::string carrier)
{
    struct Message msg;
    msg.type = MSG_LEAVE;
    msg.TTL = 3;

    ipString2Char4(carrier, msg.carrierAdd);

    sendUDP(sockfd, dest, port, (char*)&msg, sizeof(msg));
}

void join(int sockfd, std::string dest, int port)
{
	struct Message msg;
    msg.type = MSG_JOIN;
    msg.timeStamp = time(NULL);

    sendUDP(sockfd, dest, port, (char*)&msg, sizeof(msg));
}


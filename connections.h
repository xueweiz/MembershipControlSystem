#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

int bindSocket(int port);

int receiveUDP(int sockfd, char* buf, uint32_t len, std::string& sender);

void sendUDP(int sockfd, std::string& add, int port, char* buf, uint32_t len);

#endif
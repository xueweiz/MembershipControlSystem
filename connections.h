#ifndef CONNECTIONS_H
#define CONNECTIONS_H

int bindSocket(int port);

void receiveUDP(int sockfd, char* buf, uint32_t len);

void sendUDP(int sockfd, int port);

#endif
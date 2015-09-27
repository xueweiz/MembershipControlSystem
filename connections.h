#ifndef CONNECTIONS_H
#define CONNECTIONS_H

int bindSocket(int port);

void receiveUDP(int sockfd);

void sendUDP(int sockfd, int port);

#endif
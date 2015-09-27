#ifndef CONNECTIONS_H
#define CONNECTIONS_H

int bindSocket(int port);

int receiveUDP(int sockfd, char* buf, uint32_t len, std::string& sender);

void sendUDP(int sockfd, std::string& add, int port, char* buf, uint32_t len);

#endif
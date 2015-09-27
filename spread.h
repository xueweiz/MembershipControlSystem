/* Spreading part */

#ifndef SPREAD_H
#define SPREAD_H

#include <iostream>
#include <string>

void spreadFailure(int sockfd, std::string dest, int port, std::string carrier);
void spreadLeave  (int sockfd, std::string dest, int port, std::string carrier);
void join(int sockfd, std::string dest, int port);

#endif
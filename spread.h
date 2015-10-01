/* Spreading part */

#ifndef SPREAD_H
#define SPREAD_H

#include <iostream>
#include <string>

void spreadFailure(int sockfd, std::string dest, int port, std::string carrier);
void spreadLeave(int sockfd, std::string dest, int port, std::string carrier);

void ipString2Char4(std::string ip, char* buf);

void failureDetected(std::string failAdd); // This is the method we call when we detect a failure

#endif
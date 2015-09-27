/* Spreading part */

#ifndef SPREAD_H
#define SPREAD_H


void spreadFailure(int sockfd, int port, int machineId);
void spreadLeave(int sockfd, int port, int machineId);

#endif
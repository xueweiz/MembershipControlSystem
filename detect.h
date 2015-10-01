/* Detection part */
#ifndef DETECT_H
#define DETECT_H

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <thread>
#include <vector>
#include <stdlib.h>     // atoi
#include <unistd.h>     // sleep
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <mutex>

#include "connections.h"
#include "constant.h"
#include "spread.h"
#include "ChronoCpu.h"

using namespace std;

void sendPing(int sockfd, std::string dest, int port, int roundId, std::string carrier);

void sendAck(int sockfd, std::string dest, int port, int roundId, std::string carrier);

void detectThread();

void printMember();

//if already exist, return 1. else return 0
int addMember(char * carrierAdd, int timeStamp);

//check IP
int checkMember(char * carrierAdd);

//check IP + timeStamp
int checkMember(char * carrierAdd, int timeStamp);

//if already failed, return 1. else return 0
int failMember(char * carrierAdd, int timeStamp);

#endif
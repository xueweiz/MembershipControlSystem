/* Spreading part */

#ifndef SPREAD_H
#define SPREAD_H

#include <iostream>
#include <string>
#include "connections.h"
#include "constant.h"

using namespace std;

void ipString2Char4(std::string ip, char* buf);

void spreadMessage(Message msg);

void failureDetected(Node process); // This is the method we call when we detect a failure

#endif
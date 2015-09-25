#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
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
#include <fcntl.h>
#include <mutex>

#include "connections.h"
#include "grep.h"
#include "constant.h"
#include "message.h"
#include "ChronoCpu.h"

using namespace std;

std::mutex printLogLock;

int SERVER_PORT;

std::vector<std::stringstream> logs(NODES_NUMBER);
std::vector<std::string> address;
std::stringstream toFile;

void printLog(char * result, int threadId)
{
    printf("\n%s\n", result);
    toFile << result;
    return;
}

/* Get address from other nodes: */
void getAdress(std::string filename)
{
    ifstream addFile(filename);

    for (int i = 0; i < NODES_NUMBER; ++i)
    {
        std::string str;
        getline(addFile, str);
        address.push_back(str);
        std::cout << "Address " << i << ": " << str << std::endl;
    }
}


int main (int argc, char* argv[])
{
    char a;
    bool flag;

    SERVER_PORT = atoi(argv[1]);

    std::cout << std::endl << "CS425 - MP2: Distributed Logging init." << std::endl;

    int sockfd = bindSocket(SERVER_PORT);
    sendUDP(SERVER_PORT);
    receiveUDP( sockfd );

    /*Server Thread
    std::thread listeningServer(listeningThread, SERVER_PORT);
    usleep(700);
    
    listeningServer.join();
    */
    
    return 0;
}

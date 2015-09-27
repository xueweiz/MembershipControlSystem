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
#include <fcntl.h>
#include <mutex>

#include "connections.h"
#include "constant.h"
#include "spread.h"
#include "detect.h"
#include "ChronoCpu.h"

using namespace std;

std::mutex printLogLock;

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

void listeningThread(int sockfd, int port)
{
    //char buffer[BUFFER_MAX];
    struct Message msg;
    std::string sender;
    srand (time(NULL));

    while (true)
    {
        int byte_count = receiveUDP(sockfd, (char*)&msg, sizeof(msg), sender);

        printf("Received from: %s\n", sender.c_str());

        if (byte_count != sizeof(msg))
        {
            printf("Error in size receiving!!!\n");
        }

        if (msg.type == MSG_FAIL || msg.type == MSG_LEAVE)
        {
            msg.TTL--; // Keep spreading
            if (msg.TTL > 0)
            {
                for (int i = 0; i < K_FORWARD; ++i)
                {
                    int dest = rand() % NODES_NUMBER + 0;
                    std::cout << address.at(dest) << std::endl;
                    sendUDP(sockfd, address.at(dest), port, (char*)&msg, sizeof(msg));
                }
            }

        }

        printf("Message received: %d\n", msg.type);
        //break;
    }
}


int main (int argc, char* argv[])
{
    char a;
    bool flag;

    int port = atoi(argv[1]);

    std::cout << std::endl << "CS425 - MP2: Membership Protocol." ;
    std::cout << std::endl << std::endl;
    getAdress("Address.add");

    int sockfd = bindSocket( port);

    spreadFailure(sockfd, port,  0);

    /*Server Thread */
    std::thread listening(listeningThread, sockfd, port);
    usleep(700);
    
    listening.join();
    // */
    
    return 0;
}

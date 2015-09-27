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

std::vector<std::string> address;
std::stringstream toFile;
std::vector<Node> nodes;

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
        struct hostent *server = gethostbyname(str.c_str());

        struct in_addr addr;
        char ip[20];
        memcpy(&addr, server->h_addr_list[0], sizeof(struct in_addr)); 
        strcpy(ip,inet_ntoa(addr));
        struct Node newnode;
        newnode.name = str;
        std::string ip_str (ip);
        newnode.ip_str = ip_str;

        ip_str.replace(ip_str.find("."),1," ");
        ip_str.replace(ip_str.find("."),1," ");
        ip_str.replace(ip_str.find("."),1," ");
        std::stringstream ssip(ip_str);

        int a;
        ssip >> a; newnode.ip[0] = a;
        ssip >> a; newnode.ip[1] = a;
        ssip >> a; newnode.ip[2] = a;
        ssip >> a; newnode.ip[3] = a;

        nodes.push_back(newnode);
        std::cout << "Node " << i << ": " << str << " : ";
        std::cout << (int)newnode.ip[0] << ".";
        std::cout << (int)newnode.ip[1] << ".";
        std::cout << (int)newnode.ip[2] << ".";
        std::cout << (int)newnode.ip[3] << std::endl;
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

            // Get the ip address
            std::stringstream ip;
            ip << (int)msg.carrierAdd[0] << ".";
            ip << (int)msg.carrierAdd[1] << ".";
            ip << (int)msg.carrierAdd[2] << ".";
            ip << (int)msg.carrierAdd[3];

            for (int i = 0; i < nodes.size(); ++i)
            {
                if ( ip.str().compare(nodes.at(i).ip_str) == 0 ) 
                {
                    nodes.at(i).active = false;
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

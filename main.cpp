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
int port, sockfd;

int roundId;

std::vector<Message> msgQueue;
std::mutex msgQueueLock;

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
        
        std::string ip_str (ip);

        struct Node newnode;
        newnode.name = str;
        newnode.ip_str = ip_str;

        nodes.push_back(newnode);
        std::cout << "Node " << i << ": " << str << " : " << ip_str << std::endl;
    }
}



void listeningThread()
{
    //char buffer[BUFFER_MAX];
    struct Message msg;
    std::string sender;
    srand (time(NULL));

    while (true)
    {
        std::cout<< endl <<"linsten round begin"<<endl; 
        int byte_count = receiveUDP(sockfd, (char*)&msg, sizeof(msg), sender);

        printf("Received from: %s\n", sender.c_str());

        if (byte_count != sizeof(msg))
        {
            printf("Error in size receiving: Message dropped\n");
            break;
        }

        std::cout<<"received msg type and TTL: "<<msg.type<<" "<<(int)msg.TTL<<endl;
        // Get the ip address of the sender
        std::stringstream ip_ss;
        ip_ss << (int)(uint8_t)msg.carrierAdd[0] << ".";
        ip_ss << (int)(uint8_t)msg.carrierAdd[1] << ".";
        ip_ss << (int)(uint8_t)msg.carrierAdd[2] << ".";
        ip_ss << (int)(uint8_t)msg.carrierAdd[3];
        std::string ip_carrier = ip_ss.str();

        if (msg.type == MSG_FAIL || msg.type == MSG_LEAVE)
        {
            msg.TTL--; // Keep spreading
            if (msg.TTL > 0)
            {
                for (int i = 0; i < K_FORWARD; ++i)
                {
                    int dest = rand() % NODES_NUMBER + 0;
                    std::cout<<"forward msg type and TTL: "<<msg.type<<" "<<(int)msg.TTL<<endl;
                    std::cout <<"forward message to: "<< address.at(dest) << std::endl;    //zxw: later we will use nodes[] instead address[]
                    sendUDP(sockfd, nodes.at(dest).ip_str , port, (char*)&msg, sizeof(msg));
                }
            }
            
            std::cout << "Failed: " << ip_carrier << std::endl;

            for (int i = 0; i < nodes.size(); ++i)
            {
                if ( ip_carrier.compare(nodes.at(i).ip_str) == 0 ) 
                {
                    //We should also check timestamp
                    //nodes.at(i).active = false; // Remove the node from the list
                    nodes.erase(nodes.begin()+1);
                    printf("Node deleted: %s\n", ip_carrier.c_str());
                    return;
                }
            }
        }
        else if (msg.type == MSG_JOIN)
        {
            struct Node newnode;
            newnode.ip_str = sender;
            newnode.active = true;
            nodes.push_back(newnode);
            std::cout << "New Node: " << sender << std::endl;

            for (int i = 0; i < nodes.size() - 1 ; ++i) //Send to all but the last added
            {
                sendUDP(sockfd, nodes.at(i).ip_str , port, (char*)&msg, sizeof(msg));
            }
        }
        else
        {
            printf("Unknown opcode of received messade: %d\n", msg.type);
        }

        //break;
    }
}

int main (int argc, char* argv[])
{
    char a;
    bool flag;
    roundId = 0;

    std::cout << std::endl << "CS425 - MP2: Membership Protocol." ;
    std::cout << std::endl << std::endl;

    port   = atoi(argv[1]);
    sockfd = bindSocket( port);

    getAdress("Address.add");

    //failureDetected("121.122.123.124");

    //join(sockfd, "127.0.0.1", port);
    /*for (int i = 0; i < K_FORWARD; ++i)
    {
        int dest = rand() % NODES_NUMBER + 0;
        std::cout <<"send join message to: "<< address.at(dest) << std::endl;    //zxw: later we will use nodes[] instead address[]
        join(sockfd, nodes.at(dest).ip_str, port);
    }*/


    /*Server Thread */
    std::thread listening(listeningThread);
    std::thread sending(sendingThread);
    usleep(700);
    
    listening.join();
    sending.join();
    // */
    
    return 0;
}

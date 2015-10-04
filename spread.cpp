/* Spreading part */

#include <sstream>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <mutex>

#include "spread.h"
#include "connections.h"
#include "constant.h"

#include <vector>
#include <iostream>

using namespace std;

extern int port;
extern int sockfd;

extern mutex membersLock;
extern vector<Node> members;  //store members in the group

void ipString2Char4(std::string ip, char* buf) // buf must be size 4
{
    ip.replace(ip.find("."),1," ");
    ip.replace(ip.find("."),1," ");
    ip.replace(ip.find("."),1," ");
    
    std::stringstream ssip(ip);
    int a;
    ssip >> a; buf[0] = (char)a; 
    ssip >> a; buf[1] = (char)a; 
    ssip >> a; buf[2] = (char)a; 
    ssip >> a; buf[3] = (char)a; 
}

std::string char42String(char* buf) // buf must be size 4
{
    std::stringstream aux;
    aux << (unsigned int) ((uint8_t)buf[0]) << ".";
    aux << (unsigned int) ((uint8_t)buf[1]) << ".";
    aux << (unsigned int) ((uint8_t)buf[2]) << ".";
    aux << (unsigned int) ((uint8_t)buf[3]);

    return aux.str();
}

void spreadMessage(Message msg, int forwardNumber)
{
    //choose k or size-1 members
    membersLock.lock();
    std::vector<Node> selectedNode = members; 
    membersLock.unlock();

    if (selectedNode.size() < 3) return;   // Not enough nodes to do something
    // We need to remeve myself and the carrier from the possible list

    selectedNode.erase(selectedNode.begin()); // remove myself

    for (int i = 0; i < selectedNode.size(); ++i)
    {
        //std::cout << "comparing: " << selectedNode.at(i).ip_str << " " << char42String(msg.carrierAdd) << std::endl;
        if ( selectedNode.at(i).ip_str.compare(char42String(msg.carrierAdd)) == 0 )
        {
            selectedNode.erase(selectedNode.begin() + i); // remove carrier
            //std::cout << "equal" << std::endl;
            break;
        }
    }

    int k = forwardNumber == 0 ? K_FORWARD : forwardNumber;

    while (selectedNode.size() > 0 && k > 0)
    {
        int random = rand() % selectedNode.size();
        sendUDP( sockfd, selectedNode[random].ip_str, port, (char*)&msg, sizeof(Message) );
        selectedNode.erase(selectedNode.begin()+random);
        k--;
    }
}

/*
void spreadMessage(Message msg, int forwardNumber)
{
    //choose k or size-1 members
    
    vector <Node> selectedNode; 

    membersLock.lock();
    int size = members.size();
    int * selectedInt;
    int selectedSize;

    int k = forwardNumber == 0 ? K_FORWARD : forwardNumber;
    if( size-1 <= k ){
        selectedInt = new int[size-1];
        selectedSize = size-1;

        for(int i=0; i < size-1; i++)
            selectedInt[i] = i;
    }
    else{
        //randomly select k nodes   --- using shuffle method
        selectedInt = new int[k];
        selectedSize = k;

        for(int i=0; i < k; i++)
            selectedInt[i] = i;

        int pool = k;
        while(pool!=0){
            int dest = rand()%pool;
            int temp = selectedInt[k-pool];
            selectedInt[k-pool] = selectedInt[dest];
            selectedInt[dest] = temp;
            pool--;
        }
    }

    for(int i=0; i < selectedSize; i++){
        selectedNode.push_back(members[1 + selectedInt[i]]);    //+1 is because never send to itself
    }

    delete selectedInt;
    membersLock.unlock();

    //send them the msg
    for(int i=0; i < selectedSize; i++){
        sendUDP( sockfd, selectedNode[i].ip_str, port, (char*)&msg, sizeof(Message) );
    }
}
*/
void failureDetected(Node process) // This is the method we call when we detect a failure
{
    Message msg;
    msg.type = MSG_FAIL;
    msg.roundId = 0;
    ipString2Char4( process.ip_str, msg.carrierAdd );
    msg.timeStamp = process.timeStamp;
    msg.TTL = K_FORWARD;

    spreadMessage(msg);
}
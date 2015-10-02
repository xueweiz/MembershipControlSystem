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

void spreadMessage(Message msg)
{
    //choose k or size-1 members
    
    vector <Node> selectedNode; 

    membersLock.lock();
    int size = members.size();
    int * selectedInt;
    int selectedSize;

    int k = K_FORWARD;
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
        selectedNode.push_back(members[selectedInt[i]]);
    }

    delete selectedInt;
    membersLock.unlock();

    //send them the msg
    for(int i=0; i < selectedSize; i++){
        sendUDP( sockfd, selectedNode[i].ip_str, port, (char*)&msg, sizeof(Message) );
    }
}

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
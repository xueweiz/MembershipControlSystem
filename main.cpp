#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <ifaddrs.h>
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

vector<Node> nodes;    //store initial nodes
vector<std::string> address;

mutex membersLock;
vector<Node> members;  //store members in the group

int port, sockfd;   //for UDP connection

mutex roundLock;
int roundId;    //used by detect thread.

bool isIntroducer;

int myTimeStamp;
string my_ip_str;
char myAddr[4];

vector<Message> msgQueue;
mutex msgQueueLock;

/* Get address from other nodes: */
void getAdress(std::string filename)
{
    ifstream addFile(filename);

    std::string str;
    int i=0; 
    
    while(getline(addFile, str)!=0)
    {
        address.push_back(str);
        struct hostent *server = gethostbyname(str.c_str());

        struct in_addr addr;
        char ip[20];
        memcpy(&addr, server->h_addr_list[0], sizeof(struct in_addr)); 
        strcpy(ip,inet_ntoa(addr));
        
        std::string ip_str (ip);

        struct Node newnode;
        //newnode.name = str;
        newnode.ip_str = ip_str;

        nodes.push_back(newnode);
        if(isIntroducer)
        	cout<<"getAdress: Introducer ";
        else
        	cout<<"getAdress: non-Introducer ";
        cout << "read node " << i << " from addr file: " << str << " : " << ip_str << std::endl;
        i++;
    }
}

/*
	if TTL == 0
		send back an ack message ( change type to ack)
	else if TTL == 2
		send ping message to carrier addr (message carrier=sender Addr, TTL = 1)
	else if TTL == 1
		send back an ack message (TTL=2, type=ACK)
	else
		cout<<"ping message with wrong TTL"
*/
void pingMsg( Message msg, string sender ){
	if(msg.TTL == 0){
		msg.type = MSG_ACK;
		sendUDP( sockfd,  sender, port, (char*)&msg, sizeof(Message) );
	}
	else if(msg.TTL == 2){
		string target = getSenderIP(msg.carrierAdd);
		msg.TTL = 1;

		ipString2Char4(sender, msg.carrierAdd);	

		sendUDP( sockfd,  target, port, (char*)&msg, sizeof(Message) );
	}
	else if(msg.TTL == 1){
		msg.TTL = 2;
		msg.type = MSG_ACK;
		sendUDP( sockfd,  sender, port, (char*)&msg, sizeof(Message) );
	}
	else{
		cout<<"pingMsg: this ping message has illegal TTL"<<endl;
	}
}

/*
	if TTL == 0 || 1
		pushMsg()
	else if TTL == 2
		send message to carrier (message carrier=sender Addr, TTL = 1)
	else
		cout<<"ack message with wrong TTL"
*/
void ackMsg( Message msg, string sender ){
	if(msg.TTL==0 || msg.TTL==1){
		msgQueueLock.lock();
		pushMsgQueue(msg);
		msgQueueLock.unlock();
	}
	else if(msg.TTL == 2){
		string target = getSenderIP(msg.carrierAdd);
		msg.TTL = 1;

		ipString2Char4(sender, msg.carrierAdd);	

		sendUDP( sockfd,  target, port, (char*)&msg, sizeof(Message) );
	}
	else{
		cout<<"ackMsg: this ack message has illegal TTL"<<endl;
	}

}

/*
	fail the carrier node
	cout<<carrier failed
	msg.TTL--;
	spreadMessage(msg) 
*/
void failMsg( Message msg, string sender ){
	if(msg.TTL==0)
		return;
	string ip_str = getSenderIP(msg.carrierAdd);
	cout<<"failMsg: node "<<ip_str<<" failed"<<endl;
	failMember(msg.carrierAdd, msg.timeStamp);
	msg.TTL--;
	spreadMessage(msg);
}


/*
	call rejoin()	//only when me is false-failured
*/
void joinMsg( Message msg, string sender ){
	//if you get this message, it means you has been failed by other nodes.
	//this means false-failure
	//you should call rejoin()
}

/*
	fail the carrier node
	cout<<carrier failed
	msg.TTL--;
	spreadMessage(msg) 
*/
void leaveMsg( Message msg, string sender ){
	string ip_str = getSenderIP(msg.carrierAdd);
	cout<<"leaveMsg: node "<<ip_str<<" leaved"<<endl;
	failMember(msg.carrierAdd, msg.timeStamp);
	msg.TTL--;
	spreadMessage(msg);
}

void listeningThread()
{
    //char buffer[BUFFER_MAX];
    struct Message msg;
    std::string sender;
    srand (time(NULL));
    std::cout<< endl <<"listeningThread: thread begins"<<endl; 
    while (true)
    {
        int byte_count = receiveUDP(sockfd, (char*)&msg, sizeof(msg), sender);

        printf("listeningThread: Receive message from: %s\n", sender.c_str());

        if (byte_count != sizeof(msg))
        {
            printf("listeningThread: Error in size receiving: Message dropped\n");
            continue;
        }

        std::cout<<"listeningThread: received msg type and TTL: "<<msg.type<<" "<<(int)msg.TTL<<endl;
        
        if(msg.type == MSG_PING)
        	pingMsg(msg, sender);
        else if(msg.type == MSG_ACK)
        	ackMsg(msg, sender);
        else if(msg.type == MSG_FAIL)
        	failMsg(msg, sender);
        else if(msg.type == MSG_JOIN)
        	joinMsg(msg, sender);
        else if(msg.type == MSG_LEAVE)
        	leaveMsg(msg, sender);
        else{
        	cout<<"listeningThread: received msg does not belong to a type"<<endl;
        }

    }
}

int sendBackLocalList(int connFd){
	membersLock.lock();

    int size = members.size();
    
    //prepare all the members
    Message * buffer = new Message[size];
    char addr[4];

    for(int i=0; i < size; i++){
    	buffer[i].type = MSG_JOIN;
		buffer[i].roundId = -1;
		ipString2Char4(members[i].ip_str, addr);
		for(int j=0; j < 4; j++)
			buffer[i].carrierAdd[j] = addr[j];
		buffer[i].timeStamp = members[i].timeStamp;
		buffer[i].TTL = 0;
    }
    
    membersLock.unlock();

    Message sizeMsg;
    sizeMsg.timeStamp = size;
    int ret = write(connFd, &sizeMsg, sizeof(Message) );

    for(int i=0; i < size; i++){
    	ret = write(connFd, buffer+i, sizeof(Message) );
    	cout<<"sendBackLocalList: sending "<<getSenderIP(buffer[i].carrierAdd)<<" "<<buffer[i].timeStamp<<endl;
    }
    delete [] buffer;
    return 0;
}

int broadcastJoin(Message income, int i){
	income.TTL = 0;
	int connectionToServer;
    //TCP connect to members of introducer
    cout<<"broadcastJoin: Introducer trying to broadcast join to "<<members[i].ip_str<<endl;
    int ret = connect_to_server(members[i].ip_str.c_str(), port + 1, &connectionToServer);
    if(ret!=0){
        cout<<"broadcastJoin: cannot connect to "<<members[i].ip_str<<endl;
        return 1;
    }
    else{
    	ret = write(connectionToServer, &income, sizeof(Message) );
    	close(connectionToServer);
    }
    return 0;
}

void forJoinThread(){
    int listenFd = open_socket(port + 1);   //use the port next to UDP as TCP port
    while(true)
    {
        int ret;
        int connFd = listen_socket(listenFd);

        cout<<"ForJoinThread: one node asking for membership list"<<endl;

        Message income;
        char addr[4];

        read(connFd, &income, sizeof(income));
        
        //if this is normal node: 
        	//if carrier is introducer, send back local list, then add carrier node
        	//else, just add carrier node
        //if this is introducer node:
        	//send back local list. add carrier node. tell everyone carrier node is joining.


        //me is a normal node
        if(!isIntroducer){
        	//if introducer is joining
        	if(income.TTL == 1){
        		//give introducer my local list
        		sendBackLocalList(connFd);
        	}
        	//no matter what is the joining node, join it
        	addMember(income.carrierAdd, income.timeStamp);
        }
        else{	//this node is an introducer
        	
        	//send back local list
        	sendBackLocalList(connFd);
        	
        	//add carrier node
        	addMember(income.carrierAdd, income.timeStamp);
        	
        	//tell everyone carrier node is joining
        	membersLock.lock();
        	thread ** broadThreads = new thread*[members.size() ];	//members[0] is introducer itself, members[size-1] is the node just joined
        	for(int i=1; i < members.size() - 1; i++)
        		broadThreads[i] = new thread(broadcastJoin, income, i);
        	for(int i=1; i < members.size() - 1; i++){
        		broadThreads[i]->join();
        		delete broadThreads[i];
        	}
        	delete [] broadThreads;
        	membersLock.unlock();
        }

        close(connFd);
        
        printMember();
    }
    return;
}

bool firstJoin(){
    cout<<"calling firstJoin"<<endl;
   
    roundLock.lock();
    roundId = 0;
    roundLock.unlock();

    //set my own addr, ip, timeStamp, roundID
    myTimeStamp = time(NULL);
    my_ip_str = getOwnIPAddr();
    ipString2Char4(my_ip_str, myAddr);
    
    //now I have my self as member
    addMember( myAddr, myTimeStamp );

    bool joined = false;

    Message msg;
    msg.type = MSG_JOIN;
    msg.roundId = -1;
    for(int i=0; i < 4; i++)
        msg.carrierAdd[i] = myAddr[i];
    msg.timeStamp = myTimeStamp;
    msg.TTL = isIntroducer;	//used to distinguish joining node is Introducer or not

    for(int i=0; (i < nodes.size()) /*&& !joined*/ ; i++){
        int connectionToServer;
        //TCP connect to introducer/other nodes
        cout<<"FirstJoin: try to connect to "<<nodes[i].ip_str<<endl;
        int ret = connect_to_server(nodes[i].ip_str.c_str(), port + 1, &connectionToServer);
        if(ret!=0){
            cout<<"FirstJoin: cannot connect to "<<nodes[i].ip_str<<endl;
            continue;
        }
        else{
            ret = write(connectionToServer, &msg, sizeof(Message) );
            
            Message newMsg;
            read(connectionToServer, &newMsg, sizeof(Message));

            int size = newMsg.timeStamp;
            
            Message * msgs = new Message[size];

            int j=0;
            for(j=0; j < size; j++){
                read(connectionToServer, msgs + j, sizeof(Message));
                cout<<"FirstJoin: received "<<getSenderIP(msgs[j].carrierAdd)<<" "<<msgs[j].timeStamp<<endl;
            }

            if(j == size){
                joined = true;
                for(j=0; j < size; j++)
                	addMember(msgs[j].carrierAdd, msgs[j].timeStamp);
            }
            else{
            	cout<<"FirstJoin: during downloading membership list, it failed"<<endl;
            }

            delete [] msgs;
            close(connectionToServer);
        }    
    }

    printMember();

    return joined;
}

int main (int argc, char* argv[])
{
    srand (time(NULL));
    std::cout << std::endl << "CS425 - MP2: Membership Protocol." ;
    std::cout << std::endl << std::endl;

    port   = atoi(argv[1]);
    isIntroducer = atoi(argv[2]);

    sockfd = bindSocket( port);

    if(isIntroducer)
        getAdress("Address.add");
    else
        getAdress("AddrIntro.add");

    bool joined = false;
    joined = firstJoin();
    while( !isIntroducer && !joined){   //introducer will firstJoin() once. Other node will keep firstJoin() until it enter the group.
        joined = firstJoin();
        usleep( 1000*1000 );
    }



    std::thread forJoin(forJoinThread);

    std::thread listening(listeningThread);
    std::thread detecting(detectThread);
    
    forJoin.join();  
    listening.join();
    detecting.join();
  
    
    return 0;
}

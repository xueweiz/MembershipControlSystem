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

bool firstJoin();

std::vector<Node> nodes;    //store initial nodes
std::vector<std::string> address;

std::ofstream logFile;

mutex membersLock;
std::vector<Node> members;  //store members in the group

int port, sockfd;   //for UDP connection

mutex roundLock;
int roundId;    //used by detect thread.

bool isIntroducer;

int myTimeStamp;
string my_ip_str;
char myAddr[4];

std::vector<Message> msgQueue;
mutex msgQueueLock;

/* Get address from other nodes: */
void getAdress(std::string filename)
{
    std::ifstream addFile(filename);

    std::string str;
    int i=0; 
    
    while(!addFile.eof())
    {
        getline(addFile, str);
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
        	logFile<<"getAdress: Introducer ";
        else
        	logFile<<"getAdress: non-Introducer ";
        logFile << "read node " << i << " from addr file: " << str << " : " << ip_str << std::endl;
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
		logFile<<"ping message with wrong TTL"
void pingMsg( Message msg, std::string sender ){
    if(msg.TTL == 0){
        msg.type = MSG_ACK;
        sendUDP( sockfd,  sender, port, (char*)&msg, sizeof(Message) );
    }
    else if(msg.TTL == 2){
        std::string target = getSenderIP(msg.carrierAdd);
        msg.TTL = 1;

        ipString2Char4(sender, msg.carrierAdd); 

        sendUDP( sockfd,  target, port, (char*)&msg, sizeof(Message) );
        std::cout << "Weired!: " << sender << " says that " << target << " did not responde..." << std::endl;
    }
    else if(msg.TTL == 1){
        msg.TTL = 2;
        msg.type = MSG_ACK;
        sendUDP( sockfd,  sender, port, (char*)&msg, sizeof(Message) );
        //std::cout << "Weired!: I am alive. Send ack to " << sender << std::endl;
    }
    else{
        logFile<<"pingMsg: this ping message has illegal TTL"<<endl;
    }
}
*/
void pingMsg( Message msg, std::string sender )
{
	msg.type = MSG_ACK;
	sendUDP( sockfd,  sender, port, (char*)&msg, sizeof(Message) );
}

void ackMsg( Message msg, std::string sender )
{
    msgQueueLock.lock();
    pushMsgQueue(msg);
    msgQueueLock.unlock();
}

void piggyMsg( Message msg, std::string sender )
{
    std::string target = getSenderIP(msg.carrierAdd);
    msg.type = MSG_PIGGY_PING;

    ipString2Char4(sender, msg.carrierAdd); //Original Sender as carrier

    sendUDP( sockfd,  target, port, (char*)&msg, sizeof(Message) );
    //std::cout << "Weired!: " << sender << " says that " << target << " did not responde..." << std::endl;
}

void piggyPingMsg( Message msg, std::string sender )
{
    msg.type = MSG_PIGGY_ACK;
    //ipString2Char4(getSenderIP(msg.carrierAdd), msg.carrierAdd); //Original Sender as carrier
    sendUDP( sockfd,  sender, port, (char*)&msg, sizeof(Message) );

    //Just in case, we can send the message back to the original sender
    // Because we can!
    std::string original = getSenderIP(msg.carrierAdd);
    msg.type = MSG_ACK;
    sendUDP( sockfd,  original, port, (char*)&msg, sizeof(Message) );
}

void piggyAckMsg( Message msg, std::string sender )
{
    std::string target = getSenderIP(msg.carrierAdd);
    msg.type = MSG_ACK;

    ipString2Char4(sender, msg.carrierAdd); //Dont care, but meh

    sendUDP( sockfd,  target, port, (char*)&msg, sizeof(Message) );
}

/*
	if TTL == 0 || 1
		pushMsg()
	else if TTL == 2
		send message to carrier (message carrier=sender Addr, TTL = 1)
	else
		logFile<<"ack message with wrong TTL"

void ackMsg( Message msg, std::string sender ){
	if(msg.TTL==0 || msg.TTL==1){
		msgQueueLock.lock();
		pushMsgQueue(msg);
		msgQueueLock.unlock();
	}
	else if(msg.TTL == 2){
		std::string target = getSenderIP(msg.carrierAdd);
		msg.TTL = 1;

		ipString2Char4(sender, msg.carrierAdd);	

		sendUDP( sockfd,  target, port, (char*)&msg, sizeof(Message) );
        //std::cout << "OK!: " << sender << " Is alive. Notify " << target << std::endl;

	}
	else{
		logFile<<"ackMsg: this ack message has illegal TTL"<<endl;
	}

}
*/

/*
	fail the carrier node
	logFile<<carrier failed
	msg.TTL--;
	spreadMessage(msg) 
*/
void failMsg( Message msg, std::string sender )
{
	std::string carrier = getSenderIP(msg.carrierAdd);
/*
    if ( my_ip_str.compare(carrier) == 0) // THIS IS MY! I NEED TO REJOIN
    {
        std::cout <<"they are trying to kill me!" << std::endl;
        membersLock.lock();
        members.clear();
        membersLock.unlock();
        usleep( 2000*1000 ); // wait 2 second and rejoin
        bool joined = firstJoin();
        while( !isIntroducer && !joined)
        {   //introducer will firstJoin() once. Other node will keep firstJoin() until it enter the group.
            joined = firstJoin();
            usleep( 1000*1000 );
        }
        return;
    }
*/
    failMember(carrier, msg.timeStamp);
    logFile<<"failMsg: node "<<carrier<<" failed"<<endl;
	//std::cout<<"failMsg: node "<<carrier<<" failed acording to " << sender <<endl;
/*
    int select = rand()%10;

    if (select <= 7) // 1 and 2
    //if (true) // 1 and 2
    {
        Message justInCase;
        justInCase.type = MSG_FAIL;
        msg.TTL = 0;
        ipString2Char4(carrier, justInCase.carrierAdd);
        sendUDP( sockfd,  carrier, port, (char*)&justInCase, sizeof(Message) );
        // Just in case let the node know so it can rejoin
    }
*/
    if(msg.TTL==0)
    {
        return;
    }
    else{
    	msg.TTL--;
    	spreadMessage(msg);
    }
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
	logFile<<carrier failed
	msg.TTL--;
	spreadMessage(msg) 
*/
void leaveMsg( Message msg, string sender )
{
    std::string carrier = getSenderIP(msg.carrierAdd);

    if ( !checkMember(carrier) ) return; // Already deleted

	failMember(carrier, msg.timeStamp);
    
    logFile<<"leaveMsg: node "<<carrier<<" has left"<<std::endl;
    //std::cout<<"leaveMsg: node "<<carrier<<" has left"<<std::endl;

    if(msg.TTL == 0)
    {
        return;
    }
    else{
        msg.TTL--;
        spreadMessage(msg);
    }
}

void listeningThread()
{
    //char buffer[BUFFER_MAX];
    struct Message msg;
    std::string sender;
    srand (time(NULL));
    logFile<< std::endl <<"listeningThread: thread begins"<<std::endl; 
    while (true)
    {
        if (members.empty())
        {
            //In case the node decided to leave
            continue;
        }

        int byte_count = receiveUDP(sockfd, (char*)&msg, sizeof(msg), sender);

        logFile << "listeningThread: Receive message from: " << sender.c_str() << " ";
        logFile << msg.type << " " << (int)msg.TTL << std::endl;

        if (byte_count != sizeof(msg))
        {
            logFile << "ERROR listeningThread: size receiving incorrect: Message dropped" << std::endl;
            continue;
        }
        
        switch (msg.type)
        {
            case MSG_PING:  
                pingMsg(msg, sender);
                break;
            case MSG_ACK:
                ackMsg(msg, sender);
                break;
            case MSG_PIGGY:         
                piggyMsg(msg, sender);
                break;
            case MSG_PIGGY_PING:    
                piggyPingMsg(msg, sender);
                break;
            case MSG_PIGGY_ACK:     
                piggyAckMsg(msg, sender);
                break;
            case MSG_FAIL:          
                failMsg(msg, sender);
                break;
            case MSG_JOIN:          
                joinMsg(msg, sender);
                break;
            case MSG_LEAVE:         
                leaveMsg(msg, sender);
                break;
            
            default:
        	   logFile<<"ERROR: listeningThread: received msg does not belong to a type"<<std::endl;
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
    	logFile<<"sendBackLocalList: sending "<<getSenderIP(buffer[i].carrierAdd)<<" "<<buffer[i].timeStamp<<endl;
    }
    delete [] buffer;
    return 0;
}

int broadcastJoin(Message income, int i)
{
	income.TTL = 0;
	int connectionToServer;
    //TCP connect to members of introducer
    logFile<<"broadcastJoin: Introducer trying to broadcast join to "<<members[i].ip_str<<endl;
    int ret = connect_to_server(members[i].ip_str.c_str(), port + 1, &connectionToServer);
    if(ret!=0){
        logFile<<"broadcastJoin: cannot connect to "<<members[i].ip_str<<endl;
        return 1;
    }
    else{
    	ret = write(connectionToServer, &income, sizeof(Message) );
    	close(connectionToServer);
    }
    return 0;
}

void forJoinThread(){ // Will run only in the introducer
    int listenFd = open_socket(port + 1);   //use the port next to UDP as TCP port
    while(true)
    {
        int ret;
        int connFd = listen_socket(listenFd);

        logFile<<"ForJoinThread: one node asking for membership list"<<endl;

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
        	
        	usleep( 10*1000 );	//wait to make sure last joined member has enough time to open TCP listening port

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
        
        //printMember();
    }
    return;
}

bool firstJoin(){
    logFile<<"calling firstJoin"<<endl;


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
        std::cout<<"Join: Connecting to "<< nodes[i].ip_str << "..." << std::endl;
        logFile  <<"Join: Connecting to "<< nodes[i].ip_str << "..." << std::endl;
        int ret = connect_to_server(nodes[i].ip_str.c_str(), port + 1, &connectionToServer);
        if(ret!=0){
            std:cout<<"Join: Cannot connect to "<<nodes[i].ip_str<<endl;
            logFile <<"ERROR Join: Cannot connect to "<<nodes[i].ip_str<<endl;
            if(!isIntroducer){
            	i--;
            	usleep(200*1000);
            }
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
                logFile<<"FirstJoin: received "<<getSenderIP(msgs[j].carrierAdd)<<" "<<msgs[j].timeStamp<<endl;
            }

            if(j == size){
                joined = true;
                for(j=0; j < size; j++)
                	addMember(msgs[j].carrierAdd, msgs[j].timeStamp);
            }
            else{
                std::cout << "Join: Failed downloading nodes list"<<endl;
            	logFile   <<"ERROR Join: Failed downloading nodes list"<<endl;
            }

            delete [] msgs;
            close(connectionToServer);
        }    
    }

    //printMember();

    return joined;
}

/* User thread: Waits for user to input a grep command 
When receiving the grep command from command line (test cases uses this), 
it will bypass the cin*/
void listeningCin()
{
    std::string input;
    while (true)
    {

        std::cout << "Type a command (table, leave, join or quit): ";
        getline(std::cin, input);
        //std::cout << "You entered: " << input << std::endl;

        if (input.compare("quit") == 0 || input.compare("q") == 0)
        {
            std::cout << "Exiting normally " << std::endl;
            exit(0);
        }
        else if (input.compare("table") == 0 || input.compare("t") == 0)
        {
            std::cout << printMember();
        }
        else if (input.compare("leave") == 0 || input.compare("l") == 0)
        {
            membersLock.lock();
            Message msg;
            msg.type = MSG_LEAVE;
            msg.TTL = 1; // Just in case
            ipString2Char4(members.at(0).ip_str, msg.carrierAdd);
            msg.timeStamp = members.at(0).timeStamp;
            membersLock.unlock();
            spreadMessage(msg); // this method wants the lock!
            membersLock.lock();
            //members.at(0).active = false;
            members.clear();
            membersLock.unlock();
        }
        else if (input.compare("join") == 0 || input.compare("j") == 0)
        {
            bool joined = firstJoin();
            while( !isIntroducer && !joined)
            {   //introducer will firstJoin() once. Other node will keep firstJoin() until it enter the group.
                joined = firstJoin();
                //usleep( 1000*1000 );
            }
        }
        else if (input.compare("netstat") == 0 || input.compare("n") == 0)
        {
            std::cout << "UDP Stats: Sent: " << getUDPSent();
            std::cout << " Received: " << getUDPReceived() << std::endl;
        }
        else{
            std::cout << "PLEASE CHECK AGAIN THE POSSIBLE OPTIONS" << std::endl;
        }

    }

    return;
}

int main (int argc, char* argv[])
{
    std::cout << std::endl << "CS425 - MP2: Membership Protocol." ;
    std::cout << std::endl << std::endl;

    logFile.open("log.log");
    logFile << "Inicializing! " << std::endl;

    port   = atoi(argv[1]);
    isIntroducer = atoi(argv[2]);

    sockfd = bindSocket( port);

    if(isIntroducer){
        logFile << "I am the introducer! " << std::endl;
        getAdress("Address.add");
    }
    else{
        getAdress("AddrIntro.add");
    }

    roundLock.lock();
    roundId = 0;
    roundLock.unlock();

    bool joined = firstJoin();
    srand (time(NULL));

    while( !isIntroducer && !joined){   //introducer will firstJoin() once. Other node will keep firstJoin() until it enter the group.
        joined = firstJoin();
        //usleep( 1000*1000 );
    }

    std::thread forJoin(forJoinThread);

    std::thread listening(listeningThread);
    std::thread detecting(detectThread);

    /*User thread */
    usleep( 1000*1000 );
    std::thread cinListening(listeningCin);
    
    forJoin.join();  
    listening.join();
    detecting.join();
    cinListening.join();
  
    
    return 0;
}

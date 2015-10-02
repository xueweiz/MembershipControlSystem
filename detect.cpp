/* Detection part */

#include "detect.h"

extern stringstream toFile;

extern int port;
extern int sockfd;

extern mutex roundLock;
extern int roundId;
extern vector<Message> msgQueue;
extern mutex msgQueueLock;

extern mutex membersLock;
extern vector<Node> members;  //store members in the group

void printMember(){
    membersLock.lock();
    cout<<endl<<"Print members: "<<endl;
    for(int i=0; i < members.size(); i++){
        cout<<"Member["<<i<<"]: "<<members[i].ip_str<<" "<<members[i].timeStamp<<" "<<members[i].active<<endl;
    }
    cout<<endl;
    membersLock.unlock();    
}

//if already exist, return 1. else return 0
int addMember(char * carrierAdd, int timeStamp){
    Node newMember;
    newMember.ip_str = getSenderIP(carrierAdd);
    newMember.timeStamp = timeStamp;
    newMember.active = 1;
    
    membersLock.lock();

    bool exist = false;
    int position = 0;
    for(int i=0; i<members.size(); i++){
        if( members[i].ip_str.compare( newMember.ip_str )==0 ){
            exist = true;
            position = i;
        }
    }
    
    if(exist){
        members[position].timeStamp = newMember.timeStamp;
    }
    else{
        members.push_back(newMember);
    }

    membersLock.unlock();

    return exist;
}

//check IP
int checkMember(char * carrierAdd){
    bool exist = false;
    string ip_str = getSenderIP(carrierAdd);
    
    membersLock.lock();
    
    for(int i=0; i<members.size(); i++){
        if( members[i].ip_str.compare( ip_str )==0 ){
            exist = true;
        }
    }

    membersLock.unlock();

    return exist;
}

//check IP + timeStamp
int checkMember(char * carrierAdd, int timeStamp){
    bool exist = false;
    string ip_str = getSenderIP(carrierAdd);
    
    membersLock.lock();
    
    for(int i=0; i<members.size(); i++){
        if( members[i].ip_str.compare( ip_str )==0 && members[i].timeStamp == timeStamp){
            exist = true;
        }
    }

    membersLock.unlock();

    return exist;   
}

//if already failed, return 1. else return 0
int failMember(char * carrierAdd, int timeStamp){
    string ip_str = getSenderIP(carrierAdd);

    membersLock.lock();

    bool exist = false;
    int position = 0;
    for(int i=0; i<members.size(); i++){
        if( members[i].ip_str.compare( ip_str )==0 && members[i].timeStamp == timeStamp ){
            exist = true;
            position = i;
        }
    }
    
    if(exist){
        members.erase( members.begin()+position );
    }
    
    membersLock.unlock();

    return !exist;
}

bool msgQueueEmpty(){
	msgQueueLock.lock();
	bool empty;
	if(msgQueue.size()==0)
		empty = true;
	else
		empty = false;
	msgQueueLock.unlock();
	return empty;
}

Message popMsgQueue(){
	Message msg;
	msg.type= MSG_EMPTY;
	msgQueueLock.lock();

	if(msgQueue.size()!=0){
		msg = msgQueue.back();
		msgQueue.pop_back();
	}
	
	msgQueueLock.unlock();
	return msg;
}

void pushMsgQueue(Message msg){
	msgQueueLock.lock();
	msgQueue.push_back(msg);	
	msgQueueLock.unlock();
	return;
}

int queueSize(){
	msgQueueLock.lock();
	int size = msgQueue.size();
	msgQueueLock.unlock();
	return size;
}

void sendPing(int sockfd, string dest, int port, string carrier){
	struct Message msg;
	msg.roundId = roundId;
    msg.type = MSG_PING;
    msg.timeStamp = 0;
    msg.TTL = 0;

    ipString2Char4(carrier, msg.carrierAdd);

    sendUDP(sockfd, dest, port, (char*)&msg, sizeof(msg));
}

void detectThread()
{

}
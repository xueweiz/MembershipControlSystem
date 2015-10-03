/* Detection part */

#include "detect.h"

extern std::ofstream logFile;

extern int port;
extern int sockfd;

extern mutex roundLock;
extern int roundId;
extern vector<Message> msgQueue;
extern mutex msgQueueLock;

extern mutex membersLock;
extern vector<Node> members;  //store members in the group

std::string printMember(){

    std::stringstream ret;
    membersLock.lock();
    ret<<std::endl<<"Print members: "<<std::endl;
    for(int i=0; i < members.size(); i++){
        ret<<"Member["<<i<<"]: "<<members[i].ip_str<<" "<<members[i].timeStamp<<" "<<members[i].active<<std::endl;
    }
    ret<<std::endl;
    membersLock.unlock();  

    return ret.str();  
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
int checkMember(string ip_str){
    bool exist = false;
    
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
int checkMember(string ip_str, int timeStamp){
    bool exist = false;
    
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
int failMember(string ip_str, int timeStamp){

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
	bool empty;
	if(msgQueue.size()==0)
		empty = true;
	else
		empty = false;
	return empty;
}

Message popMsgQueue(){
	Message msg;
	msg.type= MSG_EMPTY;

	if(msgQueue.size()!=0){
		msg = msgQueue.back();
		msgQueue.pop_back();
	}
	
	return msg;
}

void pushMsgQueue(Message msg){
	msgQueue.push_back(msg);	
	return;
}

int queueSize(){
	int size = msgQueue.size();
	return size;
}

bool ackMsgQueue(){
    bool acked = false;
    while(!msgQueueEmpty()){
        Message receive = popMsgQueue();
        if(receive.roundId==roundId)
            acked = true;
    }
    return acked;
}

void detectThread()
{
    /*
    WHILE LOOP
        roundId++

        randomly select one node
        send ping message
        sleep(1)
        see msgQueue, search ack
        if(ack)
            cout<<alive
            sleep(4)
            continue
        
        send ping message to random K nodes (call spreadMessage())
        sleep(4)
        see msgQueue, search ack
        if(ack)
            cout<<alive
            continue
        
        if(!ack)
            cout<<fail
            delete node
            send fail message to other nodes
            continue
    */
    while(true){
        roundId++;

        logFile<<"detectThread: round: "<<roundId<<std::endl;
        logFile<<printMember();

        //logFile<<std::endl<<std::endl;
        
        if(members.size() < 2){
            usleep(5 * MAX_LATENCY);
            continue;
        }

        int select = rand()%(members.size()-1) + 1;
        Node theNode = members[select];

        Message msg;
        msg.type = MSG_PING;
        msg.TTL = 0;
        msg.roundId = roundId;
        ipString2Char4(theNode.ip_str, msg.carrierAdd);
        msg.timeStamp = 0;

        logFile<<"detectThread: checking alive or not for "<<theNode.ip_str<<" "<<theNode.timeStamp<<std::endl;
        sendUDP(sockfd, theNode.ip_str, port, (char*)&msg, sizeof(Message));

        bool acked = false;

        usleep(MAX_LATENCY);

        msgQueueLock.lock();
        acked = ackMsgQueue();
        msgQueueLock.unlock();

        if(acked){
            logFile<<"detectThread: node alive: "<<theNode.ip_str<<" "<<theNode.timeStamp<<std::endl;
            usleep(4 * MAX_LATENCY);     
            continue;       
        }

        msg.TTL = 2;
        spreadMessage(msg);
        usleep(4 * MAX_LATENCY);

        msgQueueLock.lock();
        acked = ackMsgQueue();
        msgQueueLock.unlock();

        if(acked){
            logFile<<"detectThread: second round found node alive: "<<theNode.ip_str<<" "<<theNode.timeStamp<<std::endl;
            continue;
        }
        else{
            logFile<<"detectThread: node failed: "<<theNode.ip_str<<" "<<theNode.timeStamp<<std::endl;
            
            failMember(theNode.ip_str, theNode.timeStamp);

            Message failMsg;
            failMsg.type = MSG_FAIL;
            failMsg.roundId = roundId;
            ipString2Char4(theNode.ip_str, failMsg.carrierAdd);
            failMsg.timeStamp = theNode.timeStamp;
            failMsg.TTL = 3;

            spreadMessage(failMsg);

            continue;
        }
    }
}
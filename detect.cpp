/* Detection part */

#include "detect.h"


extern vector<string> address;
extern stringstream toFile;
extern vector<Node> nodes;
extern int port;
extern int sockfd;

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
    struct Message msg;
    string sender;
    srand (time(NULL));

    while (true)
    {
        cout<< endl <<"sending round begin, round id: "<<roundId<<endl; 
        
        if(nodes.size() < 2){
            cout<<"I have no friend to send ping to. Skip round: "<<roundId<<endl;
        }
        else{
        	int getAck = false;

        	int dest = rand() % nodes.size() + 0;
        	sendPing(sockfd, nodes.at(dest).ip_str, port, "0.0.0.0");
        	cout<<"send Ping to "<<nodes.at(dest).ip_str<<endl;
        	
        	usleep(200*1000);

        	if(msgQueueEmpty()){
        		cout<<"Message Queue empty"<<endl;
        		getAck = false;
        	}
        	else{
        		while(!msgQueueEmpty()){
        			Message msg = popMsgQueue();
        			if(msg.type==MSG_EMPTY)	//empty message
        				continue;
        			else if(msg.roundId!=roundId)	//old round's message
        				continue;
        			else if(msg.type == MSG_ACK){	//ack message
        				if(msg.roundId==roundId && msg.carrierAdd[0]==0 && msg.carrierAdd[1]==0 && msg.carrierAdd[2]==0 && msg.carrierAdd[3]==0)
        					getAck = true;
        				else
        					cout<<"sendingThread: impossible message"<<endl;	
        			}			
        			else{		//should not have this message
        				cout<<"sendingThread: don't know how to do with this msg: "<<endl;
        				cout<<"received msg type round TTL: "<<msg.type<<" "<<msg.roundId<<" "<<(int)msg.TTL<<endl;
        			}
        		}
        	}

        	if(!getAck){	//did not receive ack
        		//send out k Ping message with carrierAddr to medium nodes
        		for(int i=0; i < K_FORWARD; i++){
        			int medium = rand() % nodes.size() + 0;
        			sendPing(sockfd, nodes.at(medium).ip_str, port, nodes.at(dest).ip_str);
        			cout<<"send medium Ping to "<<nodes.at(medium).ip_str<<endl;
        		}
        		
        		//wait
        		usleep(600*1000);

        		//see if acked
	        	if(msgQueueEmpty()){
	        		getAck = false;
	        	}
	        	else{
	        		while(!msgQueueEmpty()){
	        			Message msg = popMsgQueue();
	        			if(msg.type==MSG_FAIL)	//empty message
	        				continue;
	        			else if(msg.roundId!=roundId)	//old round's message
	        				continue;
	        			else if(msg.type == MSG_ACK){	//ack message
	        				if(msg.roundId==roundId && msg.carrierAdd[0]==0 && msg.carrierAdd[1]==0 && msg.carrierAdd[2]==0 && msg.carrierAdd[3]==0)
	        					getAck = true;
	        				else
	        					cout<<"sendingThread: impossible message"<<endl;	
	        			}			
	        			else{		//should not have this message
	        				cout<<"sendingThread: don't know how to do with this msg: "<<endl;
	        				cout<<"received msg type round TTL: "<<msg.type<<" "<<msg.roundId<<" "<<(int)msg.TTL<<endl;
	        			}
	        		}
	        	}
        	}
        	
        	if(!getAck){
        		cout<<"deleting node "<<nodes.at(dest).ip_str<<endl;
        		//cout<<nodes.at(dest).ip_str<<endl;
        		failureDetected(nodes.at(dest).ip_str);
        		//nodes.erase(nodes.begin()+dest);
        	}
        }
        
        roundId++;
        usleep(1000*1000);//this sleep time should be calculated using chronos
    }
}
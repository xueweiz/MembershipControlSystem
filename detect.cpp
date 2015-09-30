/* Detection part */

#include "detect.h"

extern std::mutex printLogLock;

extern std::vector<std::string> address;
extern std::stringstream toFile;
extern std::vector<Node> nodes;
extern int port;
extern int sockfd;

extern int roundId;
extern std::vector<Message> msgQueue;
extern std::mutex msgQueueLock;

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

void sendPing(int sockfd, std::string dest, int port, std::string carrier){
	struct Message msg;
	msg.roundId = roundId;
    msg.type = MSG_PING;
    msg.timeStamp = 0;
    msg.TTL = 0;

    ipString2Char4(carrier, msg.carrierAdd);

    sendUDP(sockfd, dest, port, (char*)&msg, sizeof(msg));
}

void sendingThread()
{
    struct Message msg;
    std::string sender;
    srand (time(NULL));

    while (true)
    {
        std::cout<< endl <<"sending round begin, round id: "<<roundId<<endl; 
        
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
        				std::cout<<"received msg type round TTL: "<<msg.type<<" "<<msg.roundId<<" "<<(int)msg.TTL<<endl;
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
	        				std::cout<<"received msg type round TTL: "<<msg.type<<" "<<msg.roundId<<" "<<(int)msg.TTL<<endl;
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
#ifndef CONSTANT_H
#define CONSTANT_H

typedef enum MessageType
{
    MSG_ACK,
    MSG_PING,
    MSG_PIGGY,
    MSG_FAIL,
    MSG_JOIN,
    MSG_LEAVE,
    MSG_EMPTY
} messageType;

struct Message {

	messageType type;
	uint8_t roundId;
	//int senderAdd[4]; //We dont need to send this since we can see the sender
	char carrierAdd[4];
	int timeStamp;
	char TTL;
};

struct Node {

	std::string name; // name address
	//uint8_t	 ip[4];  // IP address not neededs
	std::string ip_str;	 // IP address in string format for easy compare;
	int port;		  // not necessary, but just in case we need it
	int timeStamp;
	bool active;
};

#define BUFFER_MAX (521)
#define NODES_NUMBER (7)
#define K_FORWARD (4)

//  in ms
#define SLEEP_TIME (50)
#define MAX_LATENCY (2000)

#endif
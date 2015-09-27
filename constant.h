#ifndef CONSTANT_H
#define CONSTANT_H

typedef enum MessageType
{
    MSG_ACK,
    MSG_PING,
    MSG_PIGGY,
    MSG_FAIL,
    MSG_JOIN,
    MSG_LEAVE
} messageType;

struct Message {

	messageType type;
	uint8_t roundId;
	//int senderAdd[4]; //We dont need to send this since we can see the sender
	int carrierAdd[4];
	int timeStamp;
	char TTL;
};

struct Node {

	std::string name; // name address
	std::string add;  // IP address
	int port;		  // not necessary, but just in case we need it
	int timeStamp;
	bool active;
};

#define BUFFER_MAX (521)
#define NODES_NUMBER (7)
#define K_FORWARD (3)

//  in ms
#define SLEEP_TIME (50)
#define MAX_LATENCY (2000)

#endif
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
	int senderAdd[4];
	int carrierAdd[4];
	int timeStamp;
	char TTL;
};

#define BUFFER_MAX (521)
#define NODES_NUMBER (7)

//  in ms
#define SLEEP_TIME (50)
#define MAX_LATENCY (2000)

#endif
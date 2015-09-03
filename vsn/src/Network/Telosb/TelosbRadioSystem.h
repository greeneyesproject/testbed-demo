/*
 * TelosbRadioSystem.h
 *
 *  Created on: 06/giu/2014
 *      Author: antlab
 */
#ifndef SRC_NETWORK_TELOSB_TELOSBRADIOSYSTEM_H_
#define SRC_NETWORK_TELOSB_TELOSBRADIOSYSTEM_H_

#include <boost/thread.hpp>
#include <Network/Telosb/serialsource.h>
#include <Network/Queue.h>

class NodeNetworkSystem;
class IncomingMessageQueue;
class Message;

class TelosbRadioSystem {
public:

	TelosbRadioSystem(NodeNetworkSystem* nodeNetworkSystem,
			const char *serial_device, int baud_rate = 115200,
			int non_blocking = 0);

	void writeMessage(Message* msg);

	void flushOutgoingMessages();

private:

	const static unsigned char _DEBUG = 1;
	/* Sequence number used to prevent duplicate packets reception */
	unsigned char _seqNum;

	serial_source _serialSource;
	boost::thread _receiverThread;
	boost::thread _senderThread;

	std::vector<IncomingMessageQueue*>* _incomingMessageQueues;
	Queue<Message> _outgoingMessageQueue;

	void _receiverThreadHandler();
	void _senderThreadHandler();

};

#endif

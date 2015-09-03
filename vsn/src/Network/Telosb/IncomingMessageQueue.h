/*
 * IncomingMessageQueue.h
 *
 *  Created on: 06/giu/2014
 *      Author: antlab
 */

#ifndef SRC_NETWORK_TELOSB_INCOMINGMESSAGEQUEUE_H_
#define  SRC_NETWORK_TELOSB_INCOMINGMESSAGEQUEUE_H_

#include <Messages/Header.h>
#include <Network/MessageParser.h>

#include <opencv2/core/core.hpp>
#include <boost/thread.hpp>

#include <vector>

class NetworkNode;
class NodeNetworkSystem;
class NetworkNode;

struct message_queue_entry {
	Header header;
	int lastPacketIdx; //this is the last packet_id added
	Bitstream bitstream; //this is the complete message bitstream
	double startTime;
	double endTime;
};

class IncomingMessageQueue {
public:
	IncomingMessageQueue(NodeNetworkSystem*);

	int size();

	void addPacketToQueue(Header*, const Bitstream&);

private:
	NodeNetworkSystem* _nodeNetworkSystem;
	std::vector<message_queue_entry> _messageQueue;

	/**
	 * Mutex used to avoid concurrency in access to _messageQueue
	 */
	mutable boost::mutex _mutex;
	void deserializeAndNotify(unsigned short cur_pos);

	const static unsigned char _lastSeqNumQueueLength = 5;
	/**
	 * Vector containing the last _lastSeqNumQueueLength sequence number deserialized and notified
	 */
	std::vector<unsigned char>* _lastSeqNumCompleted;
	unsigned char _lastSeqNumQueueIdx;
};

#endif


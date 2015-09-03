#include <Network/Telosb/IncomingMessageQueue.h>
#include <Network/NodeNetworkSystem.h>
#include <Messages/DataCTAMsg.h>
#include <Messages/DataATCMsg.h>
#include <vector>

#define _DEBUG 1

using namespace std;

IncomingMessageQueue::IncomingMessageQueue(
		NodeNetworkSystem* nodeNetworkSystem) {
	_nodeNetworkSystem = nodeNetworkSystem;

	_lastSeqNumCompleted = new vector<unsigned char>(_lastSeqNumQueueLength,
			-1);
	_lastSeqNumQueueIdx = 0;
}

int IncomingMessageQueue::size() {
	boost::mutex::scoped_lock lock(_mutex);
	return _messageQueue.size();
}

/*
 * The provided header is first copied and then deleted
 */
void IncomingMessageQueue::addPacketToQueue(Header* header,
		const Bitstream& packetBitstream) {
	if (_DEBUG)
		cout << "IncomingMessageQueue::addPacketToQueue" << endl;

	boost::mutex::scoped_lock lock(_mutex);
	unsigned int queueIdx;

	if (_DEBUG)
		cout << "IncomingMessageQueue::addPacketToQueue: header: " << *header
				<< endl;

	/*
	 * If the newly arrived packet sequence number has been already deserialized it is ignored
	 */
	bool seqNumberAlreadyProcessed = false;
	for (unsigned char idx = 0; idx < _lastSeqNumQueueLength; ++idx) {
		if (_lastSeqNumCompleted->at(idx) == header->getSeqNum()) {
			seqNumberAlreadyProcessed = true;
			break;
		}
	}
	if (seqNumberAlreadyProcessed) {
		cout << "IncomingMessageQueue::addPacketToQueue: duplicate seq num. Ignoring the packet."
				<< endl;
	} else {
		/*
		 * Find a queue entry with the same sequence number as the newly arrived packet.
		 */
		for (queueIdx = 0; queueIdx < _messageQueue.size(); ++queueIdx) {
			if (header->getSeqNum()
					== _messageQueue[queueIdx].header.getSeqNum()) {
				break;
			}
		}

		if (queueIdx == _messageQueue.size()) {
			/*
			 * Create a new queue entry
			 */
			message_queue_entry newEntry;
			newEntry.header = *header;
			newEntry.lastPacketIdx = header->getPacketIdx();
			newEntry.bitstream.insert(newEntry.bitstream.begin(),
					packetBitstream.begin(), packetBitstream.end());
			newEntry.startTime = cv::getTickCount();
			if (newEntry.lastPacketIdx == 0) {
				if (_DEBUG)
					cout
							<< "IncomingMessageQueue::addPacketToQueue: create a new entry"
							<< endl;
				_messageQueue.push_back(newEntry);

				/* If the total number of packets is  */
				if (header->getNumPackets() == 1) {
					_messageQueue[queueIdx].endTime = cv::getTickCount();
					_mutex.unlock();
					deserializeAndNotify(_messageQueue.size() - 1);
				}
			} else {
				if (_DEBUG)
					cout
							<< "IncomingMessageQueue::addPacketToQueue: Lost first packet of the message, dropping this packet "
							<< endl;
			}
		} else {
			/*
			 * Append packet bitstream to the right queue entry
			 */
			int last_packet_id = _messageQueue[queueIdx].lastPacketIdx;

			if (header->getPacketIdx() == last_packet_id + 1) {
				/*
				 * Packets order respected
				 */
				if (_DEBUG)
					cout << "IncomingMessageQueue::addPacketToQueue: adding";
				_messageQueue[queueIdx].bitstream.insert(
						_messageQueue[queueIdx].bitstream.end(),
						packetBitstream.begin(), packetBitstream.end());
				if (_DEBUG)
					cout << " packet";
				_messageQueue[queueIdx].lastPacketIdx++;
				if (_DEBUG)
					cout << " to the queue" << endl;

				if (header->getPacketIdx() == header->getNumPackets() - 1) {
					_messageQueue[queueIdx].endTime = cv::getTickCount();
					_mutex.unlock();
					deserializeAndNotify(queueIdx);
				}
			} else {
				/*
				 * Packets order wrong
				 */
				if (_DEBUG)
					cout
							<< "IncomingMessageQueue::addPacketToQueue: not adding packet to the queue: ";
				if (header->getPacketIdx() == last_packet_id) {
					if (_DEBUG)
						cout << "duplicate packet" << endl;
				}
				if (header->getPacketIdx() > last_packet_id + 1) {
					if (_DEBUG)
						cout << "packet out of order" << endl;
				}
			}
		}
	}
	delete header;
}

void IncomingMessageQueue::deserializeAndNotify(unsigned short cur_pos) {
	boost::mutex::scoped_lock lock(_mutex);

	Header* header = new Header(_messageQueue[cur_pos].header);
	Bitstream* bitstream = new Bitstream(_messageQueue[cur_pos].bitstream);

	_lastSeqNumCompleted->at(_lastSeqNumQueueIdx) = header->getSeqNum();
	++_lastSeqNumQueueIdx;
	if (_lastSeqNumQueueIdx == _lastSeqNumQueueLength) {
		_lastSeqNumQueueIdx = 0;
	}

	Message* msg = NULL;

	if (_DEBUG)
		cout
				<< "IncomingMessageQueue::deserializeAndNotify: bitstream->size() = "
				<< bitstream->size() << endl;

	msg = MessageParser::parseMessage(header, bitstream);

	if (msg != NULL) {
		double rxTime = (_messageQueue[cur_pos].endTime
				- _messageQueue[cur_pos].startTime) / cv::getTickFrequency();

		switch (_messageQueue[cur_pos].header.getMsgType()) {
		case MESSAGETYPE_DATA_CTA: {
			((DataCTAMsg*) (msg))->setTxTime(rxTime);
			break;
		}
		case MESSAGETYPE_DATA_ATC: {
			((DataATCMsg*) (msg))->setTxTime(rxTime);
			break;
		}
		default:
			break;
		}

		_nodeNetworkSystem->telosMessageHandler(msg);

	}

//erase the entry from the message queue once notified
	if (_DEBUG)
		cout << "IncomingMessageQueue::deserializeAndNotify: erasing entry"
				<< endl;
	_messageQueue.erase(_messageQueue.begin() + cur_pos);
	if (_DEBUG)
		cout << "IncomingMessageQueue::deserializeAndNotify: entry erased"
				<< endl;

}


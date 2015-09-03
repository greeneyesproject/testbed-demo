#include <Network/Telosb/TelosbRadioSystem.h>
#include <Network/NetworkNode.h>
#include <Messages/Message.h>
#include <Messages/Header.h>
#include <Network/Telosb/IncomingMessageQueue.h>
#include <Network/NodeNetworkSystem.h>
#include <opencv2/core/core.hpp>
#include <vector>
#include <iostream>

using namespace std;

static char *msgs[] = { (char*) "unknown_packet_type", (char*) "ack_timeout",
		(char*) "sync", (char*) "too_long", (char*) "too_short",
		(char*) "bad_sync", (char*) "bad_crc", (char*) "closed",
		(char*) "no_memory", (char*) "unix_error" };

void stderr_msg(serial_source_msg problem) {
	cerr << "TelosbRadioSystem: Note: " << msgs[problem] << endl;
}

TelosbRadioSystem::TelosbRadioSystem(NodeNetworkSystem* nodeNetworkSystem,
		const char *serial_device, int baud_rate, int non_blocking) {
	_seqNum = 0;

	//xxx aggiungere code all'aggiunta di camere!!!!!
	_incomingMessageQueues = new vector<IncomingMessageQueue*>(1);
	_incomingMessageQueues->at(0) = new IncomingMessageQueue(nodeNetworkSystem);

	_serialSource = open_serial_source(serial_device, baud_rate, non_blocking,
			stderr_msg);
	if (_serialSource) {
		_receiverThread = boost::thread(
				&TelosbRadioSystem::_receiverThreadHandler, this);
		_senderThread = boost::thread(&TelosbRadioSystem::_senderThreadHandler,
				this);
	} else if (_DEBUG)
		cerr
				<< "TelosbRadioSystem::TelosbRadioSystem: Error while opening serial source"
				<< endl;

}

void TelosbRadioSystem::_receiverThreadHandler() {
	int len;
	Bitstream packet;

	while (1) {

		uchar* packetPtr = (uchar*) read_serial_packet(_serialSource, &len);
		packet.assign(packetPtr, packetPtr + len);
		free(packetPtr);

		/* Read packet header */
		if (_DEBUG)
			cout
					<< "TelosbRadioSystem::_receiverThreadHandler: message received!"
					<< endl;

		if (_DEBUG > 2) {
			cout << "TelosbRadioSystem::_receiverThreadHandler: packet" << endl;
			MessageParser::printBitstream(&packet);
		}
		Header* header = Header::headerFromTelosBitstream(&packet);

		packet.erase(packet.begin(),
				packet.begin() + HEADER_SIZE_TELOS_PROTO + HEADER_SIZE_TELOS_APP);

		//xxx è una porcata va sistemata
		_incomingMessageQueues->at(0)->addPacketToQueue(header, packet);

	}
}

/* Message is deleted */
void TelosbRadioSystem::_senderThreadHandler() {
	if (_DEBUG)
		cout << "TelosbRadioSystem::_senderThreadHandler" << endl;

	Message* msg;

	while (1) {
		_outgoingMessageQueue.wait_and_pop(msg);

		if (_DEBUG)
			cout << "TelosbRadioSystem::_senderThreadHandler: sending message: "
					<< *msg << endl;

		/* Max retries per packet */
		const unsigned char max_resend = 50;

		/* xxx Maximum radio packet length. Determined by hte fact that a  */
		const unsigned char max_radio_packet_length = 122;

		/* Telos reserved header */
		const unsigned char radio_header_length = 8;

		/* Maximum radio payload length. This includes also the app header */
		const unsigned char max_radio_payload_length = max_radio_packet_length
				- radio_header_length;

		/* Application specific header, equal to the header size of an ip packet, excluding the payload size */
		const unsigned char app_header_length = HEADER_SIZE_TELOS_APP;

		/* Payload effectively available in each packed */
		const unsigned char app_payload_length = max_radio_payload_length
				- app_header_length;

		/* Determine the total number of bytes to send, based on message bitstream length */
		Bitstream* appBitstream = msg->getBitStream();

		/* Number of bytes yet to be sent */
		unsigned int remainingBytes = appBitstream->size();
		if (_DEBUG)
			cout << "TelosbRadioSystem::_senderThreadHandler: bitstream size: "
					<< remainingBytes << endl;

		/* Determine the number of packets to be sent */
		unsigned short totPackets = 1;
		if (remainingBytes) {
			totPackets = ceil(
					float(remainingBytes) / (float) (app_payload_length));
		}
		if (_DEBUG)
			cout
					<< "TelosbRadioSystem::_senderThreadHandler: total packets to send "
					<< totPackets << endl;

		/* Number of bytes sent in the current packet */
		unsigned char curPacketLength = app_payload_length;

		/* Current packet index */
		uint16_t packetIdx = 0;

		/* Data payload byte index */
		uint32_t dataIdx = 0;

		vector<uchar>* serial_packet = new vector<uchar>;
		serial_packet->reserve(max_radio_packet_length);

		/* Set the sequence number */
		msg->setSeqNum(_seqNum++);
		/* Prepare the header */
		Header header(msg->getSrc(), msg->getDst(), msg->getSeqNum(),
				totPackets, packetIdx, msg->getType(), LINKTYPE_TELOS,
				remainingBytes, 0);

		bool notifySendEnd = msg->getType() == MESSAGETYPE_DATA_ATC
				|| msg->getType() == MESSAGETYPE_DATA_CTA;

		do {

			/* Clear the packet */
			serial_packet->clear();

			/* Fill packet header */
			if (remainingBytes < app_payload_length) {
				curPacketLength = remainingBytes;
			}
			remainingBytes -= curPacketLength;

			/* Insert the Radio and the App header in the packet */
			Bitstream* radioAppHeaderPacketBistream =
					header.serializeForTelosPacket(packetIdx++,
							app_header_length + curPacketLength);

			if (_DEBUG > 1) {
				cout
						<< "TelosbRadioSystem::_senderThreadHandler: radioAppHeaderPacketBistream "
						<< endl;
				MessageParser::printBitstream(radioAppHeaderPacketBistream);
			}

			serial_packet->insert(serial_packet->end(),
					radioAppHeaderPacketBistream->begin(),
					radioAppHeaderPacketBistream->end());

			/* Fill packet payload */
			serial_packet->insert(serial_packet->end(),
					appBitstream->begin() + dataIdx,
					appBitstream->begin() + dataIdx + curPacketLength);
			dataIdx += curPacketLength;

			if (_DEBUG > 1) {
				cout
						<< "TelosbRadioSystem::_senderThreadHandler: serial_packet header"
						<< endl;
				MessageParser::printBitstream(serial_packet);
			}

			int ret = -1;
			unsigned char count_resend = 0;

			unsigned long sleepInterval = 500;
			while (ret != 0 && count_resend < max_resend) {
				ret = write_serial_packet(_serialSource, serial_packet->data(),
						radio_header_length + app_header_length
								+ curPacketLength);
				if (ret != 0) { //should be ==0 for certainty
					if (_DEBUG > 1)
						cout
								<< "TelosbRadioSystem::_senderThreadHandler: retry "
								<< (int)count_resend << endl;
					count_resend++;
					usleep(sleepInterval);
					//sleepInterval += 1000;
				}
			}

			if (_DEBUG > 1)
				cout << "TelosbRadioSystem::_senderThreadHandler: packet sent!"
						<< endl;

		} while (remainingBytes);

		if (notifySendEnd)
			NodeNetworkSystem::messageSent();

		delete msg;
	}

}

/**
 * Message is deleted in senderThreadHandler
 */
void TelosbRadioSystem::writeMessage(Message* msg) {
	if (_DEBUG)
		cout << "TelosbRadioSystem::writeMessage" << endl;
	if (msg == NULL) {
		if (_DEBUG)
			cout << "TelosbRadioSystem::writeMessage: NULL message ignored"
					<< endl;
		return;
	}
	_outgoingMessageQueue.push(msg);
}

void TelosbRadioSystem::flushOutgoingMessages() {
	_outgoingMessageQueue.flush();
}


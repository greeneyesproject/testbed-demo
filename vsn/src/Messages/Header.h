/*
 *Header.h
 *
 *  Created on: 17/lug/2014
 *      Author: greeneyes
 */

#ifndef SRC_MESSAGES_HEADER_H_
#define SRC_MESSAGES_HEADER_H_

#define HEADER_SIZE_IP 21
#define HEADER_SIZE_TELOS_PROTO 8
#define HEADER_SIZE_TELOS_APP 13

#include <Network/MessageParser.h>
#include <Network/LinkType.h>
#include <TestbedTypes.h>
#include <Messages/Message.h>
#include <opencv2/core/core.hpp>
#include <boost/shared_ptr.hpp>

class NetworkNode;
class Header;

class Header {
public:

	Header() {
		_src = NULL;
		_dst = NULL;
		_seq_num = 0;
		_num_packets = 0;
		_packet_idx = 0;
		_msg_t = MESSAGETYPE_NONE;
		_linkType = LINKTYPE_UNDEF;
		_payload_size = 0;
		_sendStartTxTick = 0;
	}

	Header(NetworkNode* const src, NetworkNode* const dst,
			const uint8_t msgSeqNum, const uint16_t numPackets,
			const uint16_t packetIdx, const MessageType msgType,
			const LinkType linkType, const uint32_t payloadSize, const int64 startTxTick);

	static Header* headerFromIpBitstream(Bitstream* bitstream);
	static Header* headerFromTelosBitstream(Bitstream* const bitstream);

	Bitstream* serializeForIpPacket() const;
	Bitstream* serializeForTelosPacket(const uint16_t packetIdx,
			const uint8_t currentFramePayloadBytes) const;

	friend std::ostream& operator <<(std::ostream& os, const Header& mex);

	NetworkNode* getSrc() const {
		return _src;
	}

	NetworkNode* getDst() const {
		return _dst;
	}

	MessageType getMsgType() const {
		return _msg_t;
	}

	uint16_t getNumPackets() const {
		return _num_packets;
	}

	uint16_t getPacketIdx() const {
		return _packet_idx;
	}

	uint16_t getSeqNum() const {
		return _seq_num;
	}

	LinkType getLinkType() const {
		return _linkType;
	}

	uint32_t getPayloadSize() const {
		return _payload_size;
	}

	int64 getStartTick() const{
		return _sendStartTxTick;
	}

private:

	const static unsigned char _DEBUG = 1;

	NetworkNode* _src;
	NetworkNode* _dst;

	MessageType _msg_t;	    // Message type (see below)
	uint16_t _packet_idx;	// Packet id
	uint16_t _num_packets;	// Total number of packets composing the message
	uint8_t _seq_num;      // Sequence number (for duplicates detection)
	uint32_t _payload_size; // Number of bytes to be read
	int64 _sendStartTxTick;

	LinkType _linkType;	//Network technology/protocol

};

#endif /* PACKETHEADER_H_ */

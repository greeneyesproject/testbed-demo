#include <Messages/Header.h>
#include <netinet/in.h>
#include <Network/NetworkNode.h>

using namespace std;

ostream& operator <<(ostream& os, const Header& header) {

	os << "from: ";
	if (header._src)
		os << *(header._src);
	else
		os << "unknown";
	os << " to: ";
	if (header._dst)
		os << *(header._dst);
	else
		os << "unknown";
	os << " type: " << header.getMsgType();
	os << " packet idx: " << header.getPacketIdx();
	os << " num packets: " << header.getNumPackets();
	os << " seq num: " << header.getSeqNum();
	os << " payload size: " << header.getPayloadSize();

	return os;
}

Header::Header(NetworkNode* const src, NetworkNode* const dst,
		const uint8_t msgSeqNum, const uint16_t totPackets,
		const uint16_t packetIdx, const MessageType messageType,
		const LinkType linkType, const uint32_t payload) {

	/**
	 * Source NetworkNode. Null if the node is unknown locally
	 */
	_src = src;
	/**
	 * Destination NetworkNode. Null if the node is unknown locally
	 */
	_dst = dst,

	_msg_t = messageType;
	_packet_idx = packetIdx; // 0 to _num_packets-1
	_num_packets = totPackets;
	_seq_num = msgSeqNum; //local sequential number, to
	_payload_size = payload;

	_linkType = linkType;

}

/**
 * Decode an IP packet header bitstream and return a new header object.
 * If source or destination are unknown the relative fields are set to NULL
 * Delete the bitstream
 */
Header* Header::headerFromIpBitstream(Bitstream* bitstream) {

	/*
	 0	SeqNum
	 1	NumPktsLSB
	 2	NumPktsMSB
	 3	PktIdLSB
	 4	PktIdMSB
	 5	MsgType
	 6	SrcId
	 7	DstId
	 8	LinkType
	 9	PayloadSizeLSB
	 10	PayloadSize -
	 11	PayloadSize -
	 12	PayloadSizeMSB
	 */

	uint8_t byteIdx = 0;

	uint16_t temp16;
	uint32_t temp32;
	int64 temp64;

	/* Sequential number */
	uint8_t seq_num = bitstream->at(byteIdx++);

	/* Number of packets composing the message */
	temp16 = 0;
	temp16 |= bitstream->at(byteIdx++);
	temp16 <<= 8;
	temp16 |= bitstream->at(byteIdx++);
	uint16_t num_packets = ntohs(temp16);

	/* Packet idx */
	temp16 = 0;
	temp16 |= bitstream->at(byteIdx++);
	temp16 <<= 8;
	temp16 |= bitstream->at(byteIdx++);
	uint16_t packet_idx = ntohs(temp16);

	/* Message type */
	MessageType msg_t = (MessageType) bitstream->at(byteIdx++);

	/* Id */
	uint8_t srcId = bitstream->at(byteIdx++);
	uint8_t dstId = bitstream->at(byteIdx++);

	/* Link type */
	LinkType linkType = (LinkType) bitstream->at(byteIdx++);

	/* Payload size */
	temp32 = 0;
	temp32 |= bitstream->at(byteIdx++);
	temp32 <<= 8;
	temp32 |= bitstream->at(byteIdx++);
	temp32 <<= 8;
	temp32 |= bitstream->at(byteIdx++);
	temp32 <<= 8;
	temp32 |= bitstream->at(byteIdx++);
	uint32_t payload_size = ntohl(temp32);

	/* If the node is not present in the local NetworkNode database the pointer is null! */
	NetworkNode* src = NetworkNode::getNodeById(srcId);
	NetworkNode* dst = NetworkNode::getNodeById(dstId);

	if (_DEBUG) {
		if (!src)
			cout << "Header::headerFromIpBitstream: srcId: " << (int) srcId
					<< endl;
		if (!dst)
			cout << "Header::headerFromIpBitstream: dstId: " << (int) dstId
					<< endl;
	}

	delete bitstream;

	return new Header(src, dst, seq_num, num_packets, packet_idx, msg_t,
			linkType, payload_size);

}

/**
 * Decode aa Telosb packet header bitstream and return a new header object.
 * If source or destination are unknown the relative fields are set to NULL
 */
Header* Header::headerFromTelosBitstream(Bitstream* const bitstream) {

	/*
	 0		0x00
	 1		0x00
	 2		0x00
	 3		0x00
	 4		0x00
	 5		RadioPayloadBytes
	 6		0x00
	 7		0x47
	 ------------ Radio Payload Start ----------
	 8	SrcAmAddrLSB (ignored)
	 9	SrcAmAddrMSB (ignored)
	 10	DstAmAddrLSB (ignored)
	 11	DstAmAddrMSB (ignored)
	 12	SeqNum
	 13	NumPktsLSB
	 14	NumPktsMSB
	 15	PktIdxLSB
	 16	PktIdxMSB
	 17	MsgType
	 18	SrcId
	 19	DstId
	 20	LinkType
	 */

	uint8_t payloadSize = bitstream->at(5) - HEADER_SIZE_TELOS_APP;

	uint8_t byteIdx = 12;

	uint16_t temp16;

	/* Sequential number */
	uint8_t seq_num = bitstream->at(byteIdx++);

	/* Number of packets composing the message */
	temp16 = 0;
	temp16 |= bitstream->at(byteIdx++);
	temp16 <<= 8;
	temp16 |= bitstream->at(byteIdx++);
	uint16_t num_packets = ntohs(temp16);

	/* Packet idx */
	temp16 = 0;
	temp16 |= bitstream->at(byteIdx++);
	temp16 <<= 8;
	temp16 |= bitstream->at(byteIdx++);
	uint16_t packet_idx = ntohs(temp16);

	/* Message type */
	MessageType msg_t = (MessageType) bitstream->at(byteIdx++);

	/* Id */
	uint8_t srcId = bitstream->at(byteIdx++);
	uint8_t dstId = bitstream->at(byteIdx++);

	/* Link type */
	LinkType linkType = (LinkType) bitstream->at(byteIdx++);

	NetworkNode* src = NetworkNode::getNodeById(srcId);
	NetworkNode* dst = NetworkNode::getNodeById(dstId);

	return new Header(src, dst, seq_num, num_packets, packet_idx, msg_t,
			linkType, payloadSize);
}

Bitstream* Header::serializeForTelosPacket(const uint16_t packetIdx,
		const uint8_t currentFramePayloadBytes) const {

	/*
	 Byte	Content
	 0		0x00
	 1		0x00
	 2		0x00
	 3		0x00
	 4		0x00
	 5		RadioPayloadBytes
	 6		0x00
	 7		0x47
	 8	SrcAmAddrMSB
	 9	SrcAmAddrLSB
	 10	DstAmAddrMSB
	 11	DstAmAddrLSB
	 12	SeqNum
	 13	NumPktsLSB
	 14	NumPktsMSB
	 15	PktIdxLSB
	 16	PktIdxMSB
	 17	MsgType
	 18	SrcId
	 19	DstId
	 20	LinkType
	 */

	uint8_t byteIdx = 0;
	uint16_t temp16;

	Bitstream* bitstream = new vector<uchar>(
	HEADER_SIZE_TELOS_APP + HEADER_SIZE_TELOS_PROTO);

	/* Radio header */
	for (; byteIdx < 5; byteIdx++) {
		bitstream->at(byteIdx) = 0x00;
	}
	bitstream->at(byteIdx++) = currentFramePayloadBytes;
	bitstream->at(byteIdx++) = 0x00;
	bitstream->at(byteIdx++) = 0x47;

	assert(byteIdx == 8);

	/* App header */

	/* Am address. The bytes order is MSB first and LSB last */
	temp16 = htons(_src->getAmAddr());
	bitstream->at(byteIdx++) = ((temp16) & 0xFF);
	bitstream->at(byteIdx++) = ((temp16 >> 8) & 0xFF);

	temp16 = htons(_dst->getAmAddr());
	bitstream->at(byteIdx++) = ((temp16) & 0xFF);
	bitstream->at(byteIdx++) = ((temp16 >> 8) & 0xFF);

	/* Sequential number */
	bitstream->at(byteIdx++) = _seq_num;

	/* Number of packets composing the message */
	temp16 = htons(_num_packets);
	bitstream->at(byteIdx++) = ((temp16 >> 8) & 0xFF);
	bitstream->at(byteIdx++) = ((temp16) & 0xFF);

	/* Packet id */
	temp16 = htons(packetIdx);
	bitstream->at(byteIdx++) = ((temp16 >> 8) & 0xFF);
	bitstream->at(byteIdx++) = ((temp16) & 0xFF);

	/* Message type */
	bitstream->at(byteIdx++) = _msg_t;

	/* Id */
	bitstream->at(byteIdx++) = _src->getId();
	bitstream->at(byteIdx++) = _dst->getId();

	/* Link type */
	bitstream->at(byteIdx++) = _linkType;

	return bitstream;
}

Bitstream* Header::serializeForIpPacket() const {

	/*
	 Byte	Content
	 0	SeqNum
	 1	NumPktsLSB
	 2	NumPktsMSB
	 3	PktIdxLSB
	 4	PktIdxMSB
	 5	MsgType
	 6	SrcId
	 7	DstId
	 8	LinkType
	 9	PayloadSizeLSB
	 10	PayloadSize -
	 11	PayloadSize -
	 12	PayloadSizeMSB
	 */

	Bitstream* bitstream = new vector<uchar>(HEADER_SIZE_IP);
	uint8_t byteIdx = 0;

	uint16_t temp16;
	uint32_t temp32;
	int64 temp64;

	/* Sequential number */
	bitstream->at(byteIdx++) = _seq_num;

	/* Number of packets composing the message */
	temp16 = htons(_num_packets);
	bitstream->at(byteIdx++) = ((temp16 >> 8) & 0xFF);
	bitstream->at(byteIdx++) = ((temp16) & 0xFF);

	/* Packet id */
	temp16 = htons(_packet_idx);
	bitstream->at(byteIdx++) = ((temp16 >> 8) & 0xFF);
	bitstream->at(byteIdx++) = ((temp16) & 0xFF);

	/* Message type */
	bitstream->at(byteIdx++) = _msg_t;

	/* Id */
	bitstream->at(byteIdx++) = _src->getId();
	bitstream->at(byteIdx++) = _dst->getId();

	/* Link type */
	bitstream->at(byteIdx++) = _linkType;

	/* Payload size */
	temp32 = htonl(_payload_size);
	bitstream->at(byteIdx++) = ((temp32 >> 24) & 0xFF);
	bitstream->at(byteIdx++) = ((temp32 >> 16) & 0xFF);
	bitstream->at(byteIdx++) = ((temp32 >> 8) & 0xFF);
	bitstream->at(byteIdx++) = ((temp32) & 0xFF);

	return bitstream;
}
